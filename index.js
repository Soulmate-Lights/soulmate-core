/* global process */
const express = require("express");
const { exec, execSync } = require("child_process");
const bodyParser = require("body-parser");
const path = require("path");
const fs = require("fs").promises;
const cors = require("cors");
const os = require("os");
const encoding = "utf-8";
const port = process.env.PORT || 8081;

function execAsync(cmd) {
  return new Promise((resolve) => {
    exec(cmd, (error, stdout, stderr) => resolve({ error, stdout, stderr }));
  });
}

// TODO: Pull soulmate-core repo every build?
console.log("Compiling a first build...");
execSync("./build.sh > /dev/null");
console.log("Done. Starting the app");

const app = express();
app.use(bodyParser.json());

app.get("/", (req, res) => res.send(200));

app.options("/build", cors());
app.post("/build", cors(), async (req, res) => {
  const sketchDir = await fs.mkdtemp(os.tmpdir());
  execSync(`rsync -av ./ ${sketchDir} --exclude node_modules --exclude .git`);
  const cppFile = path.join(sketchDir, "main", "main.cpp");
  await fs.writeFile(cppFile, req.body.sketch, { encoding });
  const run = `cd ${sketchDir} && ./build.sh`;
  const { error, stderr } = await execAsync(run);

  if (error) {
    execSync(`rm -rf ${sketchDir}`);
    return res.sendStatus(500).send(stderr);
  }

  const binFile = path.join(sketchDir, "build", "soulmate.bin");
  res.sendFile(binFile, {}, () => execSync(`rm -rf ${sketchDir}`));
});

app.listen(port, () => console.log("App ready, listening on port", port));
