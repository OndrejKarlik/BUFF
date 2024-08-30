import stylisticJs from "../_tmp/eslint/node_modules/@stylistic/eslint-plugin-js/dist/index.js";
import globals from "../_tmp/eslint/node_modules/globals/index.js";
import eslint from "../_tmp/eslint/node_modules/@eslint/js/src/index.js";
import tseslint from "../_tmp/eslint/node_modules/typescript-eslint/dist/index.js";
import tsPlugin from "../_tmp/eslint/node_modules/@typescript-eslint/eslint-plugin/dist/index.js";
// Downgrade all lint errors to warnings to get more readable IDE experience
import "../_tmp/eslint/node_modules/eslint-plugin-only-warn/src/only-warn.js";
import Path from "path";

const importMetaUrl = import.meta.url.replace("file:///", "");
const pathToTsConfig = Path.dirname(importMetaUrl);
// console.log(pathToTsConfig);

export default [
    eslint.configs.all,
    ...tseslint.configs.all,

    stylisticJs.configs["all-flat"],

    {
        plugins: {
            "@stylistic/js": stylisticJs,
            "@typescript-eslint": tsPlugin,
        },

        languageOptions: {
            parserOptions: {
                project: "./tsconfig.json",
                tsconfigRootDir: pathToTsConfig,
            },
            globals: {
                ...globals.browser,
                ULTRALIGHT_VERSION_IMPL: "readonly",
                IS_DEBUG_IMPL: "readonly",
                toggleInspector: "readonly",
                getMetaStructParam: "readonly",
                setMetaStructParam: "readonly",
            },
            sourceType: "module",
        },
        linterOptions: {
            reportUnusedDisableDirectives: "error",
        },
        // extends: "eslint:all",
        rules: {
            // Turn unwanted options off:
            "no-console": "off",
            "max-lines": "off",
            "max-depth": "off",
            "max-params": "off",
            "max-statements": "off",
            "max-lines-per-function": "off",
            "max-classes-per-file": "off",
            "id-length": "off",
            "no-plusplus": "off",
            "no-magic-numbers": "off",
            "no-inline-comments": "off",
            "no-continue": "off",
            "sort-keys": "off",
            "sort-vars": "off",
            "sort-imports": "off",
            "no-ternary": "off",
            "no-undefined": "off",
            "no-use-before-define": "off", // Replaced by TS

            "one-var": ["error", "never"],

            // Possibly reconsider
            strict: "off", // use strict is unnecessary in modules
            "no-warning-comments": "off", // Turn into warnings?
            "capitalized-comments": "off", // Enable?
            "no-constant-condition": "off",
            "no-alert": "off",
            "no-bitwise": "off", // Could we specify which?
            "no-param-reassign": "off",
            "no-undef-init": "off", // initializing with undefined. We need this to prevent "no initialization" warnings

            // Possibly reconsider: change of style
            "no-else-return": "off", // Enable?"
            "func-style": "off", // ["error", "declaration"], // switch to expression to prevent hoisting?
            "func-names": "off", // Force function names? "func-names": ["warn", "as-needed"],
            "prefer-destructuring": "off",

            // ===============================================================================================
            // Stylistic
            // ===============================================================================================

            "@stylistic/js/dot-location": ["warn", "property"],
            "@stylistic/js/array-element-newline": ["warn", "consistent"],
            "@stylistic/js/array-bracket-newline": ["warn", "consistent"],
            "@stylistic/js/linebreak-style": ["warn", "windows"],
            "@stylistic/js/padded-blocks": ["warn", "never"],
            "@stylistic/js/quote-props": ["warn", "as-needed"],
            "@stylistic/js/comma-dangle": ["warn", "always-multiline"],
            "@stylistic/js/spaced-comment": [
                "warn",
                "always",
                { line: { markers: ["/"] }, block: { balanced: true } },
            ],
            "@stylistic/js/wrap-iife": ["warn", "inside"],

            // To reconsider:
            // "@stylistic/js/arrow-parens": ["warn", "as-needed"],
            "@stylistic/js/function-call-argument-newline": ["warn", "consistent"],
            "@stylistic/js/no-extra-parens": "off", // prettier automatically adds some problematic parentheses
            // "@stylistic/js/no-extra-parens": ["warn", "all", { ternaryOperandBinaryExpressions: false }],
            "@stylistic/js/brace-style": ["warn", "1tbs", { allowSingleLine: true }],
            "@stylistic/js/function-paren-newline": "off", // clang-format breaks long calls like func(\n arg1,\n arg2);
            "@stylistic/js/multiline-comment-style": "off", // Can either fully enable or fully disable multiline /* comments
            "@stylistic/js/lines-between-class-members": "off",
            "@stylistic/js/implicit-arrow-linebreak": "off", // clang-format breaks like this
            "@stylistic/js/object-property-newline": ["error", { allowAllPropertiesOnSameLine: true }],
            // "@stylistic/js/spaced-comment": "off", // prettier does not fix this automatically

            // Disable stuff that is handled by automatic formatting anyways (we do not want that distracting us in the IDE)
            "@stylistic/js/keyword-spacing": "off",
            "@stylistic/js/key-spacing": "off",
            "@stylistic/js/no-multi-spaces": "off",
            "@stylistic/js/indent": "off",
            "@stylistic/js/newline-per-chained-call": "off",
            "@stylistic/js/multiline-ternary": "off",
            "@stylistic/js/space-before-blocks": "off",
            "@stylistic/js/space-before-function-paren": "off",
            "@stylistic/js/no-trailing-spaces": "off",
            "@stylistic/js/semi-spacing": "off",
            "@stylistic/js/arrow-spacing": "off",
            "@stylistic/js/space-infix-ops": "off",
            "@stylistic/js/template-curly-spacing": "off",
            "@stylistic/js/comma-spacing": "off",
            "@stylistic/js/quotes": "off",

            // ===============================================================================================
            // Typescript
            // ===============================================================================================
            "@typescript-eslint/no-use-before-define": [
                "error",
                { functions: false, ignoreTypeReferences: true },
            ],
            "@typescript-eslint/no-magic-numbers": "off",
            "@typescript-eslint/max-params": "off",
            "@typescript-eslint/prefer-enum-initializers": "off",
            "@typescript-eslint/explicit-function-return-type": ["error", { allowIIFEs: true }],
            "@typescript-eslint/naming-convention": [
                "error",
                { selector: "default", format: ["camelCase"] },
                { selector: "enumMember", format: ["UPPER_CASE"] },
                { selector: "typeLike", format: ["PascalCase"] },
                { selector: "import", format: ["PascalCase"] },
                { selector: "variable", format: ["camelCase"] },
                { selector: "variable", modifiers: ["const"], format: ["camelCase", "UPPER_CASE"] },
            ],

            // To reconsider:
            "@typescript-eslint/triple-slash-reference": "off", // For the .d.ts files specifying C++ imports
            "@typescript-eslint/no-non-null-assertion": "off", // Maybe get rid of the ! operators
            "@typescript-eslint/prefer-destructuring": "off",
            "@typescript-eslint/promise-function-async": ["error", { checkArrowFunctions: false }],
            "@typescript-eslint/no-unnecessary-condition": "off", // Find a way to turn this on with some configuration "macros" causing constant ifs
            "@typescript-eslint/prefer-readonly-parameter-types": "off", // Is there some way to deal with all its hits?
            "@typescript-eslint/explicit-member-accessibility": "off", // Turn on back to replace #members? https://stackoverflow.com/questions/59641564/what-are-the-differences-between-the-private-keyword-and-private-fields-in-types
            "@typescript-eslint/no-misused-promises": ["error", { checksVoidReturn: false }],
        },
    },

    {
        files: ["**/*.d.ts"],
        rules: {
            "init-declarations": "off",
        },
    },
];
