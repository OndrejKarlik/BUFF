import ChildProcess from "child_process";
import Express from "express";
import FsSynchronous from "fs";
import Path from "node:path";
import Util from "util";
// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/naming-convention, @typescript-eslint/no-require-imports
const NoCache = require("nocache"); // import not possible because it does not have @types

function defaultErrorHandler(error: unknown): void {
    console.log(error);
}

function iterateDirectorySync(dir: string, fn: (path: string) => void): void {
    FsSynchronous.readdirSync(dir).forEach((file) => {
        const fullPath = Path.join(dir, file);
        const stat = FsSynchronous.statSync(fullPath);
        if (stat.isDirectory() && !file.includes("node_modules")) {
            iterateDirectorySync(fullPath, fn);
        } else {
            fn(fullPath);
        }
    });
}

function validateUrl(url: string): void {
    if (url.includes("..") || url.includes(":")) {
        throw new Error(`Invalid URL: ${url}`);
    }
}

async function init(): Promise<void> {
    const execFilePromise = Util.promisify((await import("node:child_process")).execFile);

    const MSBUILD_PATH = ChildProcess.execFileSync(
        '"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe"',
        ["-latest", "-find", "MSBuild\\**\\Bin\\MSBuild.exe"],
        { shell: true },
    )
        .toString()
        .split("\r\n")[0];

    console.log(`MSBuild path: >>${MSBUILD_PATH}<<`);

    const app = Express();
    const PORT = 80;

    app.disable("x-powered-by");
    // app.use(Helmet()); Prevents running from from WSL
    // eslint-disable-next-line @typescript-eslint/no-unsafe-call
    app.use(NoCache());
    app.use(Express.json());

    const REPO_ROOT = Path.resolve("../..");

    for (const submodule of FsSynchronous.readdirSync(REPO_ROOT, { withFileTypes: true })) {
        if (!submodule.isDirectory()) {
            continue;
        }
        for (const project of FsSynchronous.readdirSync(Path.join(REPO_ROOT, submodule.name), {
            withFileTypes: true,
        })) {
            if (!project.isDirectory()) {
                continue;
            }
            let needsCompile = true;
            const dir = Path.join(REPO_ROOT, submodule.name, project.name);

            const virtual = project.name;
            iterateDirectorySync(REPO_ROOT, (path: string) => {
                if (path.toLowerCase().endsWith(".ts") || path.toLowerCase().endsWith(".less")) {
                    console.log(`Starting to watch file ${path} for modifications.`);
                    FsSynchronous.watch(path, (eventType, filename) => {
                        console.log(
                            `File ${filename} change: '${eventType}' - forcing recompile on next JS request`,
                        );
                        needsCompile = true;
                    });
                }
            });

            console.log(`Starting virtual folder ${virtual} referencing ${dir}`);

            let promise: Promise<void> | null = null;
            app.use(
                `/${virtual}/`,
                async (req, res, next) => {
                    validateUrl(req.originalUrl);
                    if (req.originalUrl === `/${virtual}/`) {
                        // Redirect the root URL
                        res.redirect(`assets/${virtual}.html`);
                    }
                    const extension = Path.extname(req.originalUrl).toLowerCase();

                    const rebuildProject = async (sub: string, name: string): Promise<void> => {
                        await execFilePromise(
                            MSBUILD_PATH,
                            [
                                Path.resolve(
                                    REPO_ROOT,
                                    "..",
                                    "_tmp",
                                    "build_cl",
                                    sub,
                                    name,
                                    `${name}.vcxproj`,
                                ),
                                "/t:CustomBuild",
                                "/p:Configuration=Debug",
                                "-verbosity:quiet",
                                "-nologo",
                            ],
                            { windowsHide: true },
                        );
                    };

                    if (extension === ".js" || extension === ".css") {
                        if (needsCompile) {
                            try {
                                if (promise) {
                                    console.log(`Waiting fro promise: ${req.originalUrl}`);
                                    await promise;
                                } else {
                                    console.log("RECOMPILING");
                                    promise = rebuildProject(submodule.name, virtual);
                                    await rebuildProject("Buff", "LibUltralight");
                                    await promise;
                                    console.log("RECOMPILED");
                                    // eslint-disable-next-line require-atomic-updates
                                    needsCompile = false;
                                    // eslint-disable-next-line require-atomic-updates
                                    promise = null;
                                }
                            } catch (error: unknown) {
                                const stdOut: string = (error as { stdout: string }).stdout;
                                console.log(`COMPILATION ERROR!\n${stdOut}`);
                                const escaped = JSON.stringify(stdOut);
                                res.contentType("application/javascript").send(
                                    `alert("Typescript compilation failed!\\n" + ${escaped})`,
                                );
                                console.log(`ERROR: Compilation failed: ${req.originalUrl}`);
                                return;
                            }
                        }
                    }
                    console.log(req.originalUrl);
                    next();
                },
                Express.static(Path.join(REPO_ROOT, "..", "_tmp", "rundir", submodule.name, project.name)),
            );
        }
    }

    app.listen(PORT);
}

init().catch(defaultErrorHandler);
