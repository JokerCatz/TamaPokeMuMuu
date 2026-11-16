import Build from "@/components/Build";
import ChessDemo from "@/components/ChessDemo";
import Companion from "@/components/Companion";
import Follower from "@/components/Follower";
import Footer from "@/components/Footer";
import Friends from "@/components/Friends";
import Header from "@/components/Header";
import Hero from "@/components/Hero";
import Journey from "@/components/Journey";
import PixelEdge from "@/components/PixelEdge";
import Reveal from "@/components/Reveal";
import Story from "@/components/Story";
import Tournament from "@/components/Tournament";

const CREAM = "var(--color-cream)";
const GREEN = "var(--color-cgreen)";
const BLUE = "var(--color-cblue)";
const YELLOW = "var(--color-cyellow)";
const PINK = "var(--color-cpink)";

export default function Home() {
  return (
    <>
      <Header />
      <main>
        <Hero />
        <Story />
        <PixelEdge from={CREAM} to={GREEN} />
        <ChessDemo />
        <PixelEdge from={GREEN} to={YELLOW} />
        <Companion />
        <PixelEdge from={YELLOW} to={PINK} />
        <Tournament />
        <PixelEdge from={PINK} to={BLUE} />
        <Journey />
        <PixelEdge from={BLUE} to="var(--color-night)" />
        <Friends />
        <PixelEdge from="var(--color-night2)" to={CREAM} />
        <Build />
        <PixelEdge from={CREAM} to="var(--color-ink)" />
      </main>
      <Footer />
      <Follower />
      <Reveal />
    </>
  );
}
