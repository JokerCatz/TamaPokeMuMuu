import type { Metadata, Viewport } from "next";
import { Silkscreen } from "next/font/google";
import { SiteProvider } from "@/lib/state";
import "./globals.css";

const silkscreen = Silkscreen({ weight: "400", subsets: ["latin"], variable: "--font-silkscreen" });

const base = process.env.NEXT_PUBLIC_BASE_PATH ?? "";

export const metadata: Metadata = {
  title: "TamaPoke with Chess",
  description:
    "A gentle Pokémon virtual pet for the ESP32-S3 round AMOLED board that lives on real chess.com games. 1,025 Pokémon, no battles, English and 简体中文.",
  icons: { icon: `${base}/img/egg.png` },
  openGraph: {
    title: "TamaPoke with Chess",
    description: "A little friend. A whole world. A Pokémon virtual pet that lives on real chess games.",
    images: ["https://mu-muu.github.io/TamaPoke/docs/banner.png"],
  },
};

export const viewport: Viewport = { themeColor: "#f2efe1" };

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en" className={silkscreen.variable}>
      <body className="antialiased">
        <SiteProvider>{children}</SiteProvider>
      </body>
    </html>
  );
}
