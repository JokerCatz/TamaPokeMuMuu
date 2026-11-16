import Build from "@/components/Build";
import ChessDemo from "@/components/ChessDemo";
import Companion from "@/components/Companion";
import Footer from "@/components/Footer";
import Friends from "@/components/Friends";
import Header from "@/components/Header";
import Hero from "@/components/Hero";
import MeetDevice from "@/components/MeetDevice";
import Regions from "@/components/Regions";
import Reveal from "@/components/Reveal";

export default function Home() {
  return (
    <>
      <Header />
      <main>
        <Hero />
        <MeetDevice />
        <ChessDemo />
        <Regions />
        <Companion />
        <Friends />
        <Build />
      </main>
      <Footer />
      <Reveal />
    </>
  );
}
