/**
 * Based on generated Code of ChatGPT
 */

import { ContinentData, Continents, LedContinentMapping } from "./pages/Simulation/types";
import svgString from "./assets/worldmap.svg?raw"

export const CONTINENT_DEFAULTS: ContinentData = {
    North: 0,
    South: 0,
    Europe: 0,
    Africa: 0,
    Asia: 0,
    Australia: 0,
};

type LED = {
    index: number;
    x: number;
    y: number
};

type ContinentConfig = {
    name: Continents;
    selector: string;
};

const CONTINENT_CONFIG: ContinentConfig[] = [
    {
        name: Continents.North,
        selector: "#North"
    },
    {
        name: Continents.South,
        selector: "#South"
    },
    {
        name: Continents.Europe,
        selector: "#Europe"
    },
    {
        name: Continents.Africa,
        selector: "#Africa"
    },
    {
        name: Continents.Asia,
        selector: "#Asia"
    },
    {
        name: Continents.Australia,
        selector: "#Australia"
    },

];

export async function computeLedContinentMapping(
    leds: LED[],
    options?: {
        width?: number;
        height?: number;
        kernelRadius?: number;
        sigma?: number;
    }
): Promise<ContinentData[]> {
    const width = options?.width ?? 3000;
    const height = options?.height ?? 1800;
    const kernelRadius = options?.kernelRadius ?? 12;
    const sigma = options?.sigma ?? kernelRadius / 2;

    // --- 1. Prepare canvas ---
    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext("2d")!;

    // --- 2. Parse SVG ---
    const parser = new DOMParser();
    const svgDoc = parser.parseFromString(svgString, "image/svg+xml");

    // --- 3. Create masks ---
    const masks: Float32Array[] = [];

    for (const continent of CONTINENT_CONFIG) {
        // clone SVG
        const clone = svgDoc.cloneNode(true) as Document;

        // hide everything
        clone.querySelectorAll("g").forEach(el => {
            el.setAttribute("opacity", "0");
        });

        // show only this continent
        const target = clone.querySelector(continent.selector);
        if (!target) {
            throw new Error(`Selector not found: ${continent.selector}`);
        }

        // make entire subtree visible
        target.querySelectorAll("*").forEach(el => {
            (el as HTMLElement).setAttribute("opacity", "1");
            (el as HTMLElement).setAttribute("fill", "white");
            (el as HTMLElement).setAttribute("stroke", "white");
        });
        (target as HTMLElement).setAttribute("opacity", "1");

        // render to canvas
        const img = await loadImageFromSVG(serializeSVG(clone));
        ctx.clearRect(0, 0, width, height);
        ctx.drawImage(img, 0, 0, width, height);

        const imageData = ctx.getImageData(0, 0, width, height);
        const data = imageData.data;

        const mask = new Float32Array(width * height);

        for (let i = 0; i < width * height; i++) {
            const r = data[i * 4];
            const g = data[i * 4 + 1];
            const b = data[i * 4 + 2];

            mask[i] = (r + g + b) / (3 * 255.0);
        }

        masks.push(mask);
    }

    // --- 4. Gaussian kernel ---
    const { kernel, size: kSize } = createGaussianKernel(kernelRadius, sigma);
    const kHalf = Math.floor(kSize / 2);

    // --- 5. Compute LED weights ---
    const results: ContinentData[] = [];

    for (let li = 0; li < leds.length; li++) {
        const led = leds[li];
        const weights = new Float32Array(CONTINENT_CONFIG.length);

        const cx = Math.round(led.x);
        const cy = Math.round(led.y);

        for (let ky = -kHalf; ky <= kHalf; ky++) {
            for (let kx = -kHalf; kx <= kHalf; kx++) {
                const x = cx + kx;
                const y = cy + ky;

                if (x < 0 || y < 0 || x >= width || y >= height) continue;

                const kIdx = (ky + kHalf) * kSize + (kx + kHalf);
                const w = kernel[kIdx];
                const pIdx = y * width + x;

                for (let c = 0; c < masks.length; c++) {
                    weights[c] += w * masks[c][pIdx];
                }
            }
        }

        // normalize
        let sum = 0;
        for (let c = 0; c < weights.length; c++) {
            sum += weights[c];
        }

        const resultWeights: ContinentData = Object.assign({}, CONTINENT_DEFAULTS);

        if (sum > 0) {
            for (let c = 0; c < weights.length; c++) {
                resultWeights[CONTINENT_CONFIG[c].name] = Number((weights[c] / sum).toFixed(5));

            }
        } else {
            // fallback: no continent detected
            for (let c = 0; c < weights.length; c++) {
                resultWeights[CONTINENT_CONFIG[c].name] = 0;
            }
        }

        results.push(resultWeights);
    }

    return results;
}

// --- Helper: Gaussian kernel ---
function createGaussianKernel(radius: number, sigma: number) {
    const size = radius * 2 + 1;
    const kernel = new Float32Array(size * size);

    let sum = 0;

    for (let y = -radius; y <= radius; y++) {
        for (let x = -radius; x <= radius; x++) {
            const i = (y + radius) * size + (x + radius);
            const r2 = x * x + y * y;

            const value = Math.exp(-r2 / (2 * sigma * sigma));
            kernel[i] = value;
            sum += value;
        }
    }

    for (let i = 0; i < kernel.length; i++) {
        kernel[i] /= sum;
    }

    return { kernel, size };
}

// Helper: load SVG into image
function loadImageFromSVG(svg: string): Promise<HTMLImageElement> {
    return new Promise((resolve, reject) => {
        const img = new Image();
        const blob = new Blob([svg], { type: "image/svg+xml" });
        const url = URL.createObjectURL(blob);

        img.onload = () => {
            URL.revokeObjectURL(url);
            resolve(img);
        };

        img.onerror = reject;
        img.src = url;
    });
}

// Helper: serialize SVG
function serializeSVG(doc: Document): string {
    return new XMLSerializer().serializeToString(doc);
}