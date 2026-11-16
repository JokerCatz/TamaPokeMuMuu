// public/ files need the GitHub Pages base path in front (next/image is not used).
export const asset = (p: string) => `${process.env.NEXT_PUBLIC_BASE_PATH ?? ""}${p}`;
