#!/usr/bin/env node
'use strict';

const Ajv = require ('ajv');
const fs = require ('fs');
const util = require ('util');

// Convert `fs.readFile()` into a function that takes the
// same parameters but returns a promise.
const read_file = util.promisify(fs.readFile);

async function read_json (path) {
    return JSON.parse (await read_file (path));
}

async function read_schema (schema_path) {
    const ajv = new Ajv ({allErrors:true});
    return ajv.compile (await read_json (schema_path));
}

async function read_schema_and_instance (schema_path, instance_path) {
    return Promise.all ([read_schema (schema_path), read_json (instance_path)]);
}

function main () {
    const argv = require ('yargs')
        .strict ()
        .command ('$0 schema instance', 'Validate JSON against a schema', (yargs) => {
            yargs.positional('schema', {
                describe: 'The path to the JSON schema file against which the instance will be checked.',
                type: 'string',
            });
            yargs.positional('instance', {
                describe: 'The path to the JSON instance file.',
                type: 'string',
            });
        })
        .argv;

    read_schema_and_instance (argv.schema, argv.instance)
        .then (values => {
            const schema = values[0];
            const instance = values[1];

            const valid = schema (instance);
            if (!valid) {
                console.error (validate.errors);
            }
        })
        .catch (e => {
            console.error (e);
            process.exitCode = 1;
        });
}

main();
