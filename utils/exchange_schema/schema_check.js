#!/usr/bin/env node
//*           _                                 _               _     *
//*  ___  ___| |__   ___ _ __ ___   __ _    ___| |__   ___  ___| | __ *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` |  / __| '_ \ / _ \/ __| |/ / *
//* \__ \ (__| | | |  __/ | | | | | (_| | | (__| | | |  __/ (__|   <  *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_|  \___|_| |_|\___|\___|_|\_\ *
//*                                                                   *
//===- utils/exchange_schema/schema_check.js ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
'use strict';

const Ajv = require ('ajv');
const fs = require ('fs');
const strip_json_comments = require ('strip-json-comments');
const util = require ('util');

// Convert `fs.readFile()` into a function that takes the
// same parameters but returns a promise.
const read_file = util.promisify (fs.readFile);

async function read_json (path) {
    return JSON.parse(strip_json_comments (await read_file (path, 'utf-8')));
}

async function read_schema (schema_path) {
    const ajv = new Ajv ({allErrors: true});
    return ajv.compile (await read_json (schema_path));
}

async function read_schema_and_instance (schema_path, instance_path) {
    return Promise.all ([read_schema (schema_path), read_json (instance_path)]);
}

async function main (argv) {
    const args = argv.slice (2);
    if (args.length !== 2) {
        console.log (`${argv[1]} <schema> <instance>`);
        console.log ('Positionals:');
        console.log ('    schema    The path to the JSON schema file against which the instance will be checked.');
        console.log ('    instance  The path to the JSON instance file.');
        return;
    }
    const [schema, instance] = await read_schema_and_instance (args[0], args[1]);
    const result = schema (instance)
    if (!result) {
        throw new Error (JSON.stringify (schema.errors, null, 4));
    }
}

(async () => {
    await main (process.argv);
}) ().catch (e => {
    console.error (e);
    process.exitCode = 1;
});
