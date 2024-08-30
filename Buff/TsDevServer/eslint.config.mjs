import parent from "../../eslint.config.mjs";

const importMetaUrl = import.meta.url.replace("file:///", "");
const eraseIndex = importMetaUrl.search("eslint.config.mjs");
const pathToTsConfig = importMetaUrl.substring(0, eraseIndex);

export default [
    ...parent,

    {
        languageOptions: {
            parserOptions: {
                project: "./tsconfig.json",
                tsconfigRootDir: pathToTsConfig,
            },
        },
        rules: {
            "new-cap": "off",
        },
    },
];
