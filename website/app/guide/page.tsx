import type { Metadata } from "next";
import Footer from "@/components/Footer";
import Guide from "@/components/Guide";
import Header from "@/components/Header";

export const metadata: Metadata = {
  title: "Player's guide · TamaPoke with Chess",
  description:
    "How to play TamaPoke with Chess: install it, hatch the first egg, daily care, chess points, the shop, evolution, the Pokédex, tournament, gyms, fun and friends.",
};

export default function GuidePage() {
  return (
    <>
      <Header page="guide" />
      <main>
        <Guide />
      </main>
      <Footer />
    </>
  );
}
