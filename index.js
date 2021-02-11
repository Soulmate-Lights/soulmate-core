const express = require("express");
const { exec, execSync } = require("child_process");
const bodyParser = require("body-parser");
const path = require("path");
const os = require("os");
const fs = require("fs").promises;
const { promisify } = require("util");
const rimraf = require("rimraf");
const cors = require("cors");
const { v4 } = require("uuid");
const rm = promisify(rimraf);

const jobs = Math.max(1, os.cpus().length - 1);
const app = express();
app.use(bodyParser.json());
app.options("/build", cors());
const port = process.env.PORT || 8081;
const encoding = "utf-8";

function execAsync(cmd) {
  return new Promise((resolve) => {
    exec(cmd, (error, stdout, stderr) => resolve({ error, stdout, stderr }));
  });
}

var cached = false;

const compile = async (code) => {
  const id = v4();
  const sketchDir = path.join(__dirname, "output", id);
  execSync(`cp -r ./builder ${sketchDir}`);
  const mainDir = path.join(sketchDir, "main");
  await fs.writeFile(path.join(mainDir, "main.cpp"), code, { encoding });

  const command = `make -j${jobs} -C ${sketchDir} app`;
  console.time(command);
  const { error, stderr, stdout } = await execAsync(command);
  console.timeEnd(command);

  if (error) {
    console.log(error);
    execSync(`rm -rf ${sketchDir}`);
    return { error, stderr };
  }

  const binFile = path.join(sketchDir, "build", `soulmate.bin`);
  const builtBinFile = `/${id}.bin`;
  if (!cached) {
    console.log("=== Not cached yet! Caching...");
    execSync(`rm -rf ./builder/build`);
    execSync(`cp -r ${sketchDir}/build ./builder`);
    cached = true;
  }
  execSync(`cp ${binFile} ${builtBinFile}`);
  execSync(`rm -rf ${sketchDir}`);
  return { stdout, binFile: builtBinFile };
};

app.post("/build", cors(), async (req, res) => {
  const { error, stderr, binFile } = await compile(req.body.sketch);

  if (error) {
    res.status(500).send(stderr);
  } else {
    res.sendFile(binFile, {}, () => {
      execSync(`rm -rf ${binFile}`);
    });
  }
});

const sampleApp = `#define "FIRMWARE_NAME" soulmate \n #include <Soulmate.h> \n void setup(){} \n void loop(){}`;

console.log("=== Compiling sample app to get started...");
compile(sampleApp).then(() => {
  console.log("=== Compiled sample app. Copying cache.");
  app.listen(port, () => console.log("App ready, listening on port", port));
});

// try {
//   child_process.execSync("git pull");
// } catch {
//   console.log("Pulling failed");
// }
