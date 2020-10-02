#!/usr/bin/env node
//*  _           _            *
//* (_)_ __   __| | _____  __ *
//* | | '_ \ / _` |/ _ \ \/ / *
//* | | | | | (_| |  __/>  <  *
//* |_|_| |_|\__,_|\___/_/\_\ *
//*                           *
//===- utils/exchange_schema/index.js -------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
