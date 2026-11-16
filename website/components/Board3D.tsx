"use client";

// The board in its printed Poke Ball case (public/models/tamapoke_case.glb, built in
// Blender from the case's 3MF: see website/case3d/README.md). The scroll story drives
// it: `pos` is the chapter position (2.4 = between chapters 2 and 3) and the model
// turns to that chapter's pose. The mesh called "Screen" shows the picture `shot`.
import { Suspense, useEffect, useMemo, useRef, type RefObject } from "react";
import { Canvas, useFrame } from "@react-three/fiber";
import { ContactShadows, Environment, Lightformer, useGLTF } from "@react-three/drei";
import * as THREE from "three";
import { asset } from "@/lib/asset";

export type Pose = { x: number; y: number };

const MODEL = asset("/models/tamapoke_case.glb");
const WHITE = new THREE.Color("#fff");
const textures = new Map<string, THREE.Texture>();

// the lights follow the sky: dimmer and bluer in the last chapter (night)
function Lights({ pos, last }: { pos: RefObject<number>; last: number }) {
  const hemi = useRef<THREE.HemisphereLight>(null);
  const sun = useRef<THREE.DirectionalLight>(null);
  useFrame(({ scene }) => {
    const dark = THREE.MathUtils.clamp((pos.current ?? 0) - (last - 1), 0, 1);
    if (hemi.current) hemi.current.intensity = 1.7 - 1.1 * dark;
    if (sun.current) {
      sun.current.intensity = 2.6 - 1.9 * dark;
      sun.current.color.setRGB(1 - 0.35 * dark, 1 - 0.25 * dark, 1);
    }
    scene.environmentIntensity = 1 - 0.65 * dark;
  });
  return (
    <>
      <hemisphereLight ref={hemi} args={["#ffffff", "#aab0bd", 1.7]} />
      <directionalLight ref={sun} position={[3, 5, 4]} intensity={2.6} />
    </>
  );
}

function Case({ shot, pos, poses, onTap, onReady }: { shot: string; pos: RefObject<number>; poses: Pose[]; onTap: () => void; onReady: () => void }) {
  const { scene } = useGLTF(MODEL);
  useEffect(onReady, [onReady]); // the model has loaded
  const group = useRef<THREE.Group>(null);
  const screenMat = useMemo(() => new THREE.MeshBasicMaterial({ color: "#000", toneMapped: false }), []);

  // centre the model on its bounding box, scale it to about 2.6 units and dress the screen
  const model = useMemo(() => {
    const root = scene.clone(true);
    const box = new THREE.Box3().setFromObject(root);
    const size = box.getSize(new THREE.Vector3());
    const centre = box.getCenter(new THREE.Vector3());
    const holder = new THREE.Group();
    root.position.sub(centre);
    holder.add(root);
    holder.scale.setScalar(2.6 / Math.max(size.x, size.y, size.z));
    root.traverse((o) => {
      const m = o as THREE.Mesh;
      if (!m.isMesh || m.name !== "Screen") return;
      m.material = screenMat;
      m.userData.screen = true;
      // the glass over the picture: black, so it only adds the reflections
      const n = new THREE.Vector3().fromBufferAttribute(m.geometry.attributes.normal as THREE.BufferAttribute, 0);
      const glass = new THREE.Mesh(
        m.geometry,
        new THREE.MeshStandardMaterial({
          color: "#000",
          roughness: 0.05,
          metalness: 0,
          transparent: true,
          blending: THREE.AdditiveBlending,
          depthWrite: false,
        }),
      );
      glass.position.copy(n).multiplyScalar(0.0003);
      glass.raycast = () => {};
      m.add(glass);
    });
    return holder;
  }, [scene, screenMat]);

  // the picture: the old one stays until the new one has loaded, then a short dip like a screen change
  useEffect(() => {
    let alive = true;
    const show = (t: THREE.Texture) => {
      if (!alive) return;
      screenMat.map = t;
      screenMat.color.setScalar(0.2);
      screenMat.needsUpdate = true;
    };
    const cached = textures.get(shot);
    if (cached) show(cached);
    else
      new THREE.TextureLoader().load(shot, (t) => {
        t.colorSpace = THREE.SRGBColorSpace;
        t.flipY = false; // glTF texture coordinates
        t.anisotropy = 8;
        textures.set(shot, t);
        show(t);
      });
    return () => {
      alive = false;
    };
  }, [shot, screenMat]);

  useFrame((state, dt) => {
    const g = group.current;
    if (!g) return;
    const p = THREE.MathUtils.clamp(pos.current ?? 0, 0, poses.length - 1);
    const i = Math.min(poses.length - 2, Math.floor(p));
    // holds each chapter's pose and turns in the middle of the way to the next
    const f = THREE.MathUtils.smoothstep(p - i, 0.15, 0.85);
    const tx = THREE.MathUtils.lerp(poses[i].x, poses[i + 1].x, f) - state.pointer.y * 0.06;
    const ty = THREE.MathUtils.lerp(poses[i].y, poses[i + 1].y, f) + state.pointer.x * 0.1;
    const k = 1 - Math.exp(-6 * dt);
    g.rotation.x += (tx - g.rotation.x) * k;
    g.rotation.y += (ty - g.rotation.y) * k;
    g.position.y = Math.sin(state.clock.elapsedTime * 1.3) * 0.035;
    if (screenMat.map) screenMat.color.lerp(WHITE, 1 - Math.exp(-9 * dt));
  });

  return (
    <group ref={group} rotation={[poses[0].x, poses[0].y, 0]}>
      <primitive
        object={model}
        onClick={(e: { object: THREE.Object3D; stopPropagation: () => void }) => {
          if (!e.object.userData.screen) return;
          e.stopPropagation();
          onTap();
        }}
        onPointerOver={(e: { object: THREE.Object3D }) => {
          if (e.object.userData.screen) document.body.style.cursor = "pointer";
        }}
        onPointerOut={() => (document.body.style.cursor = "")}
      />
    </group>
  );
}

useGLTF.preload(MODEL);

export default function Board3D({
  shot,
  pos,
  poses,
  onTap,
  onReady,
  active,
}: {
  shot: string;
  pos: RefObject<number>;
  poses: Pose[];
  onTap: () => void;
  onReady: () => void;
  active: boolean; // off screen: stop drawing
}) {
  return (
    <Canvas
      frameloop={active ? "always" : "never"}
      camera={{ position: [0, 0.1, 5.3], fov: 36 }}
      dpr={[1, 2]}
      // neutral tone mapping keeps the red of the case red (the default one turns it orange)
      gl={{ antialias: true, alpha: true, toneMapping: THREE.NeutralToneMapping }}
    >
      <Lights pos={pos} last={poses.length - 1} />
      <Suspense fallback={null}>
        <Case shot={shot} pos={pos} poses={poses} onTap={onTap} onReady={onReady} />
        {/* studio panels for the reflections on the plastic and the glass (no image to download) */}
        <Environment resolution={256}>
          <Lightformer form="rect" intensity={3} position={[0, 5, 2]} scale={[9, 3, 1]} target={[0, 0, 0]} />
          <Lightformer form="rect" intensity={1.6} position={[-6, 1, 2]} scale={[5, 4, 1]} target={[0, 0, 0]} />
          <Lightformer form="rect" intensity={1.3} position={[6, 0.5, 1]} scale={[5, 4, 1]} target={[0, 0, 0]} />
          <Lightformer form="circle" intensity={1.2} position={[0, -1, 7]} scale={3} target={[0, 0, 0]} />
        </Environment>
        <ContactShadows position={[0, -1.5, 0]} opacity={0.4} scale={6} blur={2.4} far={3} resolution={512} color="#10200f" />
      </Suspense>
    </Canvas>
  );
}
