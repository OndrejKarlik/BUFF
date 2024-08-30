import Path from "path";
import Fs from "fs";
import ChildProcess from "child_process";

// ===========================================================================================================
// Utilities
// ===========================================================================================================

function getRepositoryDir(): string {
    return Path.dirname(Path.dirname(Path.dirname(__filename)));
}

function getTmpDir(): string {
    const currentDir = getRepositoryDir();
    const result = Path.join(currentDir, "..", "_tmp");
    // console.log("getTmpDir", result);
    return result;
}

function getNpmPackagesDir(): string {
    return `file://${Path.join(getTmpDir(), "node", "node_modules")}`;
}

function getRunDir(): string {
    const result = Path.join(getTmpDir(), "rundir");
    // console.log("getRunDir", result);
    return result;
}

type Compiler = "cl" | "clang" | "emscripten";

function getBuildDir(compiler: Compiler, isTest = false): string {
    const result = Path.join(getTmpDir(), (isTest ? "build_test_" : "build_") + compiler);
    // console.log("getBuildDir", result);
    return result;
}

function getMsBuild(): string {
    let res =
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\amd64\\MsBuild.exe";
    if (!Fs.existsSync(res)) {
        res =
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\amd64\\MsBuild.exe";
    }
    if (!Fs.existsSync(res)) {
        console.log("ERROR: msbuild not found!");
        process.exit(1);
    }
    return res;
}

interface ExecuteOptions {
    cwd?: string;
    printOutput?: boolean;
    shell?: boolean;
}

function prepareOptions(options: ExecuteOptions): ChildProcess.SpawnOptions {
    const stdio: ChildProcess.IOType = (options.printOutput ?? true) ? "inherit" : "ignore";
    const encoding: BufferEncoding = "utf8";
    const optionsPrepared = {
        cwd: options.cwd ?? getRepositoryDir(),
        encoding,
        stdio,
        shell: options.shell ?? false,
    };
    return optionsPrepared;
}

/// Currently just the return code
type ProcessResult = number;

async function executePromise(
    command: string,
    args: string[],
    options: ExecuteOptions = {},
): Promise<ProcessResult> {
    return new Promise((resolve, reject) => {
        const result = ChildProcess.spawn(command, args, prepareOptions(options));
        result.on("exit", (code) => {
            if (code === 0) {
                resolve(code);
            } else {
                reject(new Error(`Exit code: ${code}`));
            }
        });
    });
}

function execute(command: string, args: string[], options: ExecuteOptions = {}): void {
    ChildProcess.execFileSync(command, args, prepareOptions(options));
}

// ===========================================================================================================
// Commands
// ===========================================================================================================

async function build(compiler: Compiler, configuration: string, isTest = false): Promise<ProcessResult> {
    const msbuild = getMsBuild();
    return executePromise(msbuild, [
        Path.join(getBuildDir(compiler, isTest), `BUFF_${compiler}.sln`),
        `-p:Configuration=${configuration}`,
        "-m",
    ]);
}

async function runCmake(compiler: Compiler, isTest = false): Promise<void> {
    const TARGET_DIR = getBuildDir(compiler, isTest);
    if (!Fs.existsSync(TARGET_DIR)) {
        Fs.mkdirSync(TARGET_DIR);
    }

    if (compiler === "emscripten") {
        await executePromise(
            Path.join(getTmpDir(), "emscripten", "upstream", "emscripten", "emcmake.bat"),
            [
                "cmake",
                "-S",
                getRepositoryDir(),
                "-DCOMPILER=emscripten",
                "-DCMAKE_BUILD_TYPE=Release",
                "-G Ninja",
                `-DCMAKE_MAKE_PROGRAM='${getRepositoryDir()}/buff/.tools/ninja/ninja.exe'`,
                // #, "-target", "EmscriptenHelloWorld"
            ],
            { cwd: TARGET_DIR, shell: true },
        );
    } else {
        await executePromise(
            "cmake",
            [getRepositoryDir(), `-DCOMPILER=${compiler}`, `-DWARNINGS_AS_ERRORS=${isTest ? 1 : 0}`],
            {
                cwd: TARGET_DIR,
            },
        );
        const slnWithoutExtension = Path.join(TARGET_DIR, `BUFF_${compiler}`);
        const sln = `${slnWithoutExtension}.sln`;
        const resharper = `${sln}.DotSettings`;
        if (Fs.existsSync(resharper)) {
            console.log(`Skipping creating symlink (already exists): ${resharper}.`);
        } else {
            console.log(`Creating symlink for: ${resharper}.`);
            Fs.symlinkSync(`${getRepositoryDir()}\\buff\\BuffResharper.DotSettings`, resharper);
        }
        const originalSlnContent = Fs.readFileSync(`${slnWithoutExtension}_original.sln`, {
            encoding: "utf8",
        });
        const previousModifiedSlnContent = Fs.existsSync(sln)
            ? Fs.readFileSync(sln, {
                  encoding: "utf8",
              })
            : "";

        console.log("Adding 'Solution Items' to SLN");
        // Note: order of items in the following string is the same as VS automatically reoders items into.
        const SOLUTION_FILES = [
            "CmakeLists.txt",
            "Buff\\cpp.hint",
            "eslint.config.mjs",
            "Buff\\buff.natvis",
            ".prettierrc",
        ];
        const replacement = `
Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{409863A2-AC49-40E9-B4E7-2563E75FB0DF}"
\tProjectSection(SolutionItems) = preProject
${SOLUTION_FILES.reduce((acc, file) => `${acc}\t\t${getRepositoryDir()}\\${file} = ${getRepositoryDir()}\\${file}\n`, "")}
\tEndProjectSection
EndProject
Global
`;
        // Need more modern JS to get replaceAll on String. We need to replace a single occurence anyways
        const modifiedSlnContent = originalSlnContent.replace("\r\nGlobal\r\n", replacement);

        if (modifiedSlnContent === previousModifiedSlnContent) {
            console.log("No .sln changes since last run.");
        } else {
            console.log("Writing new .sln solution.");
            Fs.writeFileSync(sln, modifiedSlnContent);
        }
    }
}

async function runTests(compiler: Compiler, configuration: string): Promise<void> {
    console.log(`Building: ${compiler} ${configuration}`);
    await build(compiler, configuration, true);

    const promises: Promise<ProcessResult>[] = [];
    const testNames: string[] = [];
    const buildDir = getBuildDir(compiler, true);

    for (const i of Fs.readdirSync(buildDir, { recursive: true })) {
        const binaryPath = i as string;
        // Path is in format BuildDir/Compiler/Submodule/Project/Configuration/Binary.Test.exe
        const parsed = Path.parse(binaryPath);
        if (!binaryPath.endsWith(".Test.exe")) {
            continue;
        }
        const dirs = parsed.dir.split(Path.sep);
        if (dirs[dirs.length - 1] !== configuration) {
            continue;
        }
        const submodule = dirs[dirs.length - 3];
        const project = dirs[dirs.length - 2];
        const cwd = Path.join(getTmpDir(), "rundir", submodule, project);
        if (!Fs.existsSync(cwd)) {
            throw Error(`Test working directory does not exist: ${cwd}`);
        }
        promises.push(executePromise(Path.join(buildDir, binaryPath), [], { cwd }));
        testNames.push(binaryPath);
    }
    if (promises.length < 2) {
        throw Error("Something is wrong - we found less than 2 tests");
    }
    await Promise.all(promises);

    console.log("SUCCESS!");
    console.log(`Ran tests:\n${testNames.join("\n")}`);
}

async function buildEmscripten(): Promise<ProcessResult> {
    return executePromise("cmake", ["--build", "."], { cwd: getBuildDir("emscripten") });
}

function clangFormat(): void {
    execute(Path.join(getRepositoryDir(), "buff", ".tools", "clang-format", "CodeFormatter.exe"), [
        Path.join(getRepositoryDir(), "buff", ".tools", "clang-format", "clang-format.exe"),
        Path.join(getTmpDir(), ".clang-format-cache"),
        getRepositoryDir(),
    ]);
}

async function distribute(args: string[]): Promise<void> {
    if (args.length !== 2) {
        throw Error("Expected exactly 2 argument - submodule and project name");
    }
    const submodule = args[0];
    const project = args[1];
    // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment
    const fsExtra = await import(Path.join(getNpmPackagesDir(), "fs-extra", "lib", "index.js"));
    const rundir = getRunDir();
    if (Fs.existsSync(rundir)) {
        if (rundir.length > 5 && rundir.includes("_tmp")) {
            // Unnecessary arbitrary checks just son we don't get e.g. empty path and delete everything on the HDD
            Fs.rmSync(rundir, { recursive: true });
        } else {
            console.log(`Something is wrong with getRunDir!\n${rundir}`);
            process.exit(1);
        }
    }
    await runCmake("cl");
    await build("cl", "Release");
    const date = new Date();
    const yyyy = date.getFullYear();
    const mm = `${date.getMonth() + 1}`.padStart(2, "0");
    const dd = `${date.getDate()}`.padStart(2, "0");
    const hour = `${date.getHours()}`.padStart(2, "0");
    const min = `${date.getMinutes()}`.padStart(2, "0");

    const formattedTime = `${yyyy}-${mm}-${dd} ${hour}-${min}`;
    const folderName = `cl Release ${formattedTime}`;
    const outDir = Path.join(getTmpDir(), "_dist", submodule, project, folderName);

    // We need this to deal with symlinks
    // eslint-disable-next-line @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access
    await fsExtra.copy(Path.join(getRunDir(), submodule, project), outDir, {
        recursive: true,
        dereference: true,
        overwrite: true,
    });
    Fs.copyFileSync(
        Path.join(getBuildDir("cl"), submodule, project, "Release", `${project}.exe`),
        Path.join(outDir, `${project}.exe`),
    );
}

function setup(): void {
    console.log("Checking for necessary dependencies...");
    function checkApp(name: string, installUrl: string): void {
        try {
            console.log(`Searching for '${name}'`);
            execute(name, ["--version"], { printOutput: false, shell: true });
        } catch (e) {
            console.log(
                `ERROR: Missing tool '${
                    name
                }' not installed or not in %PATH%, please install it and try again.\nYou can get it here: ${
                    installUrl
                    // eslint-disable-next-line @typescript-eslint/restrict-template-expressions
                }\n\nError: ${e}`,
            );
            process.exit(1);
        }
    }

    checkApp("git", "https://git-scm.com/downloads");
    checkApp("cmake", "https://cmake.org/download/");
    checkApp("node", "https://nodejs.org/en/download/package-manager");
    checkApp("npm", "https://nodejs.org/en/download/package-manager");

    console.log("Adding git alias 'git ui'...");
    execute("git", ["config", "--global", "alias.ui", "!TortoiseGitProc /command:log /path:. &"]);

    // These are just the executables invoked directly by our tooling and IDEs.
    // There are other packages installed into dependencies later
    console.log("Installing global node.js tooling packages");
    execute("npm", ["install", "--global", "typescript", "eslint", "prettier", "less"], {
        shell: true,
    });

    console.log("Lowering CPU priority of compiler processes...");
    execute("start", ["Buff/Scripts/Setup.reg"], { shell: true });

    console.log("Setup done!");
}

async function dependencies(): Promise<void> {
    // Emscripten needs to go first, because it is a dependency of vcpkg
    const emscriptenDir = Path.join(getTmpDir(), "emscripten");
    if (!Fs.existsSync(Path.join(emscriptenDir, ".git"))) {
        execute("git", ["clone", "https://github.com/emscripten-core/emsdk.git", emscriptenDir]);
    }
    execute("git", ["pull", "-p"], { cwd: emscriptenDir });
    execute("emsdk.bat", ["install", "latest"], { cwd: emscriptenDir, shell: true });
    execute("emsdk.bat", ["activate", "latest"], { cwd: emscriptenDir, shell: true });

    // Rest can be done in parallel

    const vcpkg = async (): Promise<void> => {
        const vcpkgDir = Path.join(getTmpDir(), "vcpkg");
        if (!Fs.existsSync(Path.join(vcpkgDir, ".git"))) {
            await executePromise("git", ["clone", "https://github.com/microsoft/vcpkg.git", vcpkgDir]);
        }
        await executePromise("git", ["pull", "-p"], { cwd: vcpkgDir });
        await executePromise("bootstrap-vcpkg.bat", [], { cwd: vcpkgDir, shell: true });
        await executePromise(
            `..\\emscripten\\emsdk_env.bat && vcpkg install "@${getRepositoryDir()}/Buff/Scripts/vcpkgDeps.txt"`,
            [],
            {
                cwd: vcpkgDir,
                shell: true,
            },
        );
    };

    const ultralight = async (): Promise<void> => {
        // 1.3 stable
        async function downloadStable(): Promise<void> {
            const ultralightDir = Path.join(getTmpDir(), "ultralight");
            if (!Fs.existsSync(Path.join(ultralightDir, ".git"))) {
                await executePromise("git", [
                    "clone",
                    "https://github.com/ultralight-ux/Ultralight.git",
                    ultralightDir,
                ]);
            }
            await executePromise("git", ["pull", "-p"], { cwd: ultralightDir });
            await executePromise("cmake", ["."], { cwd: ultralightDir });
            await executePromise(getMsBuild(), ["Samples.sln", "/target:CopySDK"], { cwd: ultralightDir });
        }

        // 1.4 beta
        async function downloadBeta(url: string, outputFolder: string): Promise<void> {
            const zipResponse = await fetch(url);
            const zip = await zipResponse.arrayBuffer();
            const zipPath = Path.join(getTmpDir(), `${outputFolder}.7z`);
            const outputPath = Path.join(getTmpDir(), outputFolder);
            if (!Fs.existsSync(outputPath)) {
                Fs.mkdirSync(outputPath);
            }
            Fs.writeFileSync(zipPath, new Uint8Array(zip));
            await executePromise("tar", ["-xf", zipPath], { cwd: outputPath, shell: true });
        }
        await Promise.all([
            downloadStable(),
            downloadBeta(
                "https://ultralight-sdk-dev.sfo2.cdn.digitaloceanspaces.com/ultralight-sdk-latest-win-x64.7z",
                "ultralight-sdk-latest-win-x64",
            ),
            downloadBeta(
                "https://github.com/ultralight-ux/Ultralight/releases/download/v1.4.0-beta/ultralight-sdk-1.4.0b-win-x64-debug.7z",
                "ultralight-sdk-1.4.0b-win-x64-debug",
            ),
        ]);
    };

    // Install packages for eslint used in cmdline and IDE. They are referenced ONLY in eslint.config.mjs.
    //  Note that if we want to keep the heavy node_modules folder out of the repository,
    // we must use awkward relative paths to this folder.
    const eslintPackagesDir = Path.join(getTmpDir(), "eslint");
    if (!Fs.existsSync(eslintPackagesDir)) {
        Fs.mkdirSync(eslintPackagesDir);
    }
    const eslintPackages = executePromise(
        "npm",
        [
            "install",
            "eslint",
            "@eslint/js",
            "@stylistic/eslint-plugin-js",
            "globals",
            "eslint-plugin-only-warn",
            "@types/eslint__js",
            "typescript-eslint",
        ],
        { cwd: eslintPackagesDir, shell: true },
    );

    // Install packages for our standalone node TS projects.
    // TODO: Can we merge this and the previous node folder?
    const nodePackagesDir = Path.join(getTmpDir(), "node");
    if (!Fs.existsSync(nodePackagesDir)) {
        Fs.mkdirSync(nodePackagesDir);
    }
    const nodePackages = executePromise(
        "npm",
        [
            "install",
            "@types/node",

            // Used by TsDevServer
            "express",
            "helmet",
            "nocache",
            "@types/express",
            "@types/helmet",

            // Used by this script
            "fs-extra",
        ],
        { cwd: nodePackagesDir, shell: true },
    );

    await Promise.all([vcpkg(), ultralight(), eslintPackages, nodePackages]);
}

async function openEditor(compiler: Compiler): Promise<void> {
    const slnPath = Path.join(getBuildDir(compiler), `BUFF_${compiler}.sln`);
    if (!Fs.existsSync(slnPath)) {
        await runCmake(compiler, false);
    }
    ChildProcess.exec(`start "" "${slnPath}"`);
}

function runHttpServer(): void {
    const cwd = Path.join(getRepositoryDir(), "Buff", "TsDevServer");
    // execute("npm", ["install"], { cwd, shell: true });
    execute("tsc", [], { cwd, shell: true });
    console.log(`Running TS httpServer on http://localhost. Log files are in ${getTmpDir()}`);

    const out = Fs.openSync(Path.join(getTmpDir(), "TS_HTTP_Server_log.txt"), "a");
    const err = Fs.openSync(Path.join(getTmpDir(), "TS_HTTP_Server_err.txt"), "a");
    const subProcess = ChildProcess.spawn("node", ["src/TsDevServer.js"], {
        cwd,
        stdio: ["ignore", out, err],
        detached: true,
        // eslint-disable-next-line @typescript-eslint/naming-convention
        env: { ...process.env, NODE_PATH: `${getTmpDir()}/node/node_modules` },
    });
    subProcess.unref();
}

interface Command {
    command: string;
    action: (args: string[]) => void | Promise<void>;
}

const COMMANDS: Command[] = [
    { command: "setup", action: setup },
    { command: "dependencies", action: dependencies },
    { command: "format", action: clangFormat },
    {
        command: "eslint",
        action: async (): Promise<void> => {
            await executePromise("eslint", ["**/*.ts"], { shell: true });
        },
    },
    { command: "httpServer", action: runHttpServer },
    { command: "cmake", action: async () => runCmake("cl") },
    { command: "cmakeClang", action: async () => runCmake("clang") },
    { command: "cmakeEmscripten", action: async () => runCmake("emscripten") },
    { command: "editor", action: async () => openEditor("cl") },
    { command: "editorClang", action: async () => openEditor("clang") },
    {
        command: "test",
        action: async (): Promise<void> => {
            await runCmake("cl", true);
            await runTests("cl", "Debug");
        },
    },
    {
        command: "testAll",
        action: async (): Promise<void> => {
            const testCompiler = async (compiler: Compiler): Promise<void> => {
                await runCmake(compiler, true);
                // We cannot currently run compilation of Debug and Release in parallel, because of the web projects inside them
                await runTests(compiler, "Debug");
                await runTests(compiler, "Release");
            };
            await Promise.all([
                runCmake("emscripten").then(() => buildEmscripten()),
                testCompiler("cl"),
                testCompiler("clang"),
            ]);
            console.log("SUCCESS!");
        },
    },
    {
        command: "buildEmscripten",
        action: async (): Promise<void> => {
            await runCmake("emscripten");
            await buildEmscripten();
        },
    },
    { command: "distribute", action: distribute },
];

const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const WHITE = "\x1b[37m";

async function run(command: Command, args: string[]): Promise<void> {
    console.log(`Running command '${command.command}'`);
    const time = new Date().getTime();
    const getElapsedTime = (): string => `${(new Date().getTime() - time) / 1000} s`;
    try {
        await command.action(args);
        console.log(`${GREEN}Command '${command.command}' succeeded after ${getElapsedTime()}${WHITE}`);
    } catch (e) {
        console.log(
            // eslint-disable-next-line @typescript-eslint/restrict-template-expressions
            `${RED}ERROR: Command '${command.command}' failed after ${getElapsedTime()}${WHITE}\n${e}`,
        );
        process.exit(1);
    }
}

const args = process.argv.slice(2);
if (args.length === 0) {
    console.log("No command provided");
    process.exit(1);
}
const command = COMMANDS.find((i) => args[0] === i.command);
if (command) {
    run(command, args.slice(1)).catch(() => {
        console.log("SHOULD NOT HAPPEN");
    });
} else {
    console.log(`ERROR: unknown command '${args[0]}'!`);
    console.log("Allowed commands:");
    for (const i of COMMANDS) {
        console.log(`    * ${i.command}`);
    }
}
