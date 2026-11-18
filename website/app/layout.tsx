import type { Metadata, Viewport } from "next";
import Script from "next/script";
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
        {/* Visit counter (GoatCounter): no cookies, nothing personal. Every page is a full
            load (plain links), so count.js counts each one by itself. The same counter is in
            web/index.html. */}
        <Script
          data-goatcounter="https://tamapoke-chess.goatcounter.com/count"
          src="https://gc.zgo.at/count.v4.js"
          integrity="sha384-nRw6qfbWyJha9LhsOtSb2YJDyZdKvvCFh0fJYlkquSFjUxp9FVNugbfy8q1jdxI+"
          crossOrigin="anonymous"
          strategy="afterInteractive"
        />
      </body>
    </html>
  );
}
