"use client";

// The board in its printed Poke Ball case (public/models/tamapoke_case.glb, built
// in Blender from the case's 3MF: see website/case3d/README.md). The mesh called
// "Screen" shows real screenshots; tapping it shows the next one.
import { Suspense, useEffect, useMemo, useRef } from "react";
import { Canvas, useFrame, useLoader } from "@react-three/fiber";
import { OrbitControls, useGLTF } from "@react-three/drei";
import * as THREE from "three";
import { asset } from "@/lib/asset";

const MODEL = asset("/models/tamapoke_case.glb");

function Case({ url, onTap }: { url: string; onTap: () => void }) {
  const { scene } = useGLTF(MODEL);
  const tex = useLoader(THREE.TextureLoader, url);
  const sway = useRef<THREE.Group>(null);

  // centre the model on its bounding box and scale it to about 2.6 units across
  const model = useMemo(() => {
    const root = scene.clone(true);
    const box = new THREE.Box3().setFromObject(root);
    const size = box.getSize(new THREE.Vector3());
    const centre = box.getCenter(new THREE.Vector3());
    const s = 2.6 / Math.max(size.x, size.y, size.z);
    const holder = new THREE.Group();
    root.position.sub(centre);
    holder.add(root);
    holder.scale.setScalar(s);
    return holder;
  }, [scene]);

  useEffect(() => {
    tex.colorSpace = THREE.SRGBColorSpace;
    tex.flipY = false; // glTF texture coordinates
    tex.anisotropy = 8;
    tex.needsUpdate = true;
    model.traverse((o) => {
      const m = o as THREE.Mesh;
      if (!m.isMesh) return;
      if (m.name === "Screen") {
        m.material = new THREE.MeshBasicMaterial({ map: tex, transparent: true, toneMapped: false });
        m.userData.screen = true;
      }
    });
  }, [model, tex]);

  useFrame(({ clock }) => {
    const t = clock.getElapsedTime();
    if (sway.current) {
      // seen from the front and a little above: the screen and the ball under it
      // swings slowly between facing the screen and showing the red/white ball from the side
      sway.current.rotation.y = Math.sin(t * 0.4) * 0.55;
      sway.current.rotation.x = -0.95 + Math.sin(t * 0.25) * 0.3;
    }
  });

  return (
    <group ref={sway}>
      <primitive
        object={model}
        onClick={(e: { object: THREE.Object3D; stopPropagation: () => void }) => {
          if (e.object.userData.screen) {
            e.stopPropagation();
            onTap();
          }
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

export default function DeviceCanvas({ url, onTap }: { url: string; onTap: () => void }) {
  return (
    <Canvas camera={{ position: [0, 0, 5], fov: 38 }} dpr={[1, 2]} gl={{ antialias: true, alpha: true }}>
      <hemisphereLight args={["#ffffff", "#b8bcc8", 1.1]} />
      <directionalLight position={[3, 4, 5]} intensity={1.8} />
      <directionalLight position={[-4, -1, 3]} intensity={0.6} color="#bcd4ff" />
      <Suspense fallback={null}>
        <Case url={url} onTap={onTap} />
      </Suspense>
      <OrbitControls enableZoom={false} enablePan={false} minPolarAngle={Math.PI / 4} maxPolarAngle={(3 * Math.PI) / 4} />
    </Canvas>
  );
}
