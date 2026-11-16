// Copies the board screenshots from ../docs into public/,
// so the site and the READMEs share one set of images.
import { cpSync, mkdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const site = join(dirname(fileURLToPath(import.meta.url)), "..");
const docs = join(site, "..", "docs");
mkdirSync(join(site, "public"), { recursive: true });
cpSync(join(docs, "screenshots"), join(site, "public", "shots"), { recursive: true });
console.log("assets synced from ../docs");
