const fetch = require("node-fetch");
const fs = require("fs");
const path = require("path");
const sketch = fs.readFileSync(
  path.join(__dirname, "builder", "main", "main.cpp"),
  "utf8"
);
const url = process.argv[2] || "http://localhost:8081/build";
console.log(`Testing against Soulmate at ${url}`);

require("./index");

const test = (label, condition) => {
  console.log(condition ? "✅" : "❌", label);
  if (!condition) process.exit(1);
};

const fetchResult = (sketch) =>
  fetch(url, {
    headers: {
      accept: "*/*",
      "accept-language": "en-US",
      "cache-control": "max-age=0",
      "content-type": "application/json",
    },
    referrerPolicy: "no-referrer-when-downgrade",
    body: JSON.stringify({ sketch, board: "mega" }),
    method: "POST",
    mode: "cors",
  });

const runTests = async () => {
  let response, size, json;

  console.time("⏱", "Compilation");
  response = await fetchResult(sketch);
  size = parseInt(response.headers.get("content-length"));
  test(`Built size was ${size}`, size > 1320000 && size < 1321000);
  console.timeEnd("⏱", "Compilation");

  console.time("⏱", "Build failure");
  response = await fetchResult("Deliberately failing");
  size = parseInt(response.headers.get("content-length"));
  test(`Built size was ${size}`, size == 253);
  console.timeEnd("⏱", "Build failure");

  console.log("Testing parallel builds...");
  console.time("Parallel");
  await Promise.all([
    fetchResult(sketch),
    fetchResult(sketch),
    fetchResult(sketch),
    fetchResult(sketch),
    fetchResult(sketch),
  ]).then((values) => {
    console.timeEnd("Parallel");
    return Promise.all(
      values.map(async (response) => {
        // test("Parallel hex file is 13702 bytes", json.hex.length === 13702);
        // test("Size", response.headers.get("content-length") === 1320384);
        size = parseInt(response.headers.get("content-length"));
        test("Size", size > 1320000 && size < 1321000);
      })
    );
  });

  process.exit();
};

console.log("Running tests...");
runTests();
