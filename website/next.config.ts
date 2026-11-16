import type { NextConfig } from "next";
import { dirname } from "node:path";
import { fileURLToPath } from "node:url";

// GitHub Pages serves the site under /TamaPoke; local dev runs at the root.
const base = process.env.PAGES_BASE_PATH ?? "";

const nextConfig: NextConfig = {
  output: "export",
  // there is another package-lock.json in the home folder; this project is its own root
  outputFileTracingRoot: dirname(fileURLToPath(import.meta.url)),
  basePath: base || undefined,
  trailingSlash: true,
  images: { unoptimized: true },
  env: { NEXT_PUBLIC_BASE_PATH: base },
};

export default nextConfig;
