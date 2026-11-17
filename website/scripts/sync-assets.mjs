// Takes the board screenshots from ../docs (the ones the READMEs use) into public/shots
// as WebP, so the site and the READMEs share one set of pictures. Screens with a
// landscape shrink to a quarter as lossy WebP; flat menus are smaller lossless, so
// each file keeps whichever comes out smaller.
import { mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import sharp from "sharp";

const site = join(dirname(fileURLToPath(import.meta.url)), "..");
const src = join(site, "..", "docs", "screenshots");
let count = 0;
let before = 0;
let after = 0;
for (const lang of readdirSync(src)) {
  const files = readdirSync(join(src, lang)).filter((f) => f.endsWith(".png"));
  if (!files.length) continue;
  mkdirSync(join(site, "public", "shots", lang), { recursive: true });
  for (const f of files) {
    const png = readFileSync(join(src, lang, f));
    const [lossy, lossless] = await Promise.all([
      sharp(png).webp({ quality: 86, effort: 6 }).toBuffer(),
      sharp(png).webp({ lossless: true, effort: 6 }).toBuffer(),
    ]);
    const out = lossy.length < lossless.length ? lossy : lossless;
    writeFileSync(join(site, "public", "shots", lang, f.replace(/\.png$/, ".webp")), out);
    count++;
    before += png.length;
    after += out.length;
  }
}
console.log(`screenshots: ${count} from ../docs as WebP, ${Math.round(before / 1024)} KB -> ${Math.round(after / 1024)} KB`);
