/**
 * Lightweight Thin Plate Spline (TPS) Implementierung in TypeScript
 * Vollständig von Gemini generiert, basierend auf der mathematischen Theorie von TPS.
 */
export class ThinPlateSpline {
	private sourcePoints: number[][];

	/** Gewichte für die X-Transformation (Lon) */
	private wX: number[];

	/** Gewichte für die Y-Transformation (Lat) */
	private wY: number[];

	constructor(sourcePoints: number[][], targetPoints: number[][]) {
		this.sourcePoints = sourcePoints;
		const p = sourcePoints.length;

		// Matrix L aufbauen (Größe: p+3 x p+3)
		const L = this.zeros(p + 3, p + 3);

		for (let i = 0; i < p; i++) {
			for (let j = 0; j < p; j++) {
				L[i][j] = this.baseFunc(this.dist(sourcePoints[i], sourcePoints[j]));
			}

			// Affiner Teil in der Matrix
			L[i][p] = 1;
			L[i][p + 1] = sourcePoints[i][0];
			L[i][p + 2] = sourcePoints[i][1];

			L[p][i] = 1;
			L[p + 1][i] = sourcePoints[i][0];
			L[p + 2][i] = sourcePoints[i][1];
		}

		// Ziel-Vektoren aufbauen (jeweils für Ziel-X und Ziel-Y)
		const V_x = new Array(p + 3).fill(0);
		const V_y = new Array(p + 3).fill(0);
		for (let i = 0; i < p; i++) {
			V_x[i] = targetPoints[i][0];
			V_y[i] = targetPoints[i][1];
		}

		// Gleichungssysteme lösen (L * W = V)
		this.wX = this.solve(this.cloneMatrix(L), V_x);
		this.wY = this.solve(this.cloneMatrix(L), V_y);
	}

	// Wandelt einen einzelnen Punkt um: [x, y](Reale Position der LED-Karte) -> [lon, lat](Weltkoordinaten)
	public transform(x: number, y: number): [number, number] {
		const p = this.sourcePoints.length;

		// Affiner Startwert
		let tx = this.wX[p] + (this.wX[p + 1] * x) + (this.wX[p + 2] * y);
		let ty = this.wY[p] + (this.wY[p + 1] * x) + (this.wY[p + 2] * y);

		// Nicht-lineare (gebogene) Einflüsse der Ankerpunkte addieren
		for (let i = 0; i < p; i++) {
			const U = this.baseFunc(this.dist([x, y], this.sourcePoints[i]));
			tx += this.wX[i] * U;
			ty += this.wY[i] * U;
		}

		// Gibt [Lon, Lat] zurück
		return [tx, ty];
	}

	// --- Hilfsfunktionen für die Mathematik ---

	// Euklidischer Abstand zwischen zwei Punkten
	private dist(p1: number[], p2: number[]) {
		const dx = p1[0] - p2[0];
		const dy = p1[1] - p2[1];
		return Math.sqrt((dx * dx) + (dy * dy));
	}

	// Radial Basis Function: U(r) = r^2 * ln(r^2)
	private baseFunc(r: number) {
		if (r === 0) return 0;
		return r * r * Math.log(r * r);
	}

	// Erstellt eine leere Matrix (mit Nullen gefüllt)
	private zeros(rows: number, cols: number) {
		return Array.from({ length: rows }, () => new Array(cols).fill(0));
	}

	// Kopiert eine Matrix
	private cloneMatrix(m: number[][]) {
		return m.map(row => [...row]);
	}

	// Gauss-Elimination: Löst lineare Gleichungssysteme (Ax = b)
	private solve(A: number[][], b: number[]): number[] {
		const n = b.length;
		for (let p = 0; p < n; p++) {
			// Pivot-Element suchen (für mathematische Stabilität)
			let max = p;
			for (let i = p + 1; i < n; i++) {
				if (Math.abs(A[i][p]) > Math.abs(A[max][p])) max = i;
			}

			// Zeilen tauschen
			const tempA = A[p]; A[p] = A[max]; A[max] = tempA;
			const tempB = b[p]; b[p] = b[max]; b[max] = tempB;

			// Eliminieren
			for (let i = p + 1; i < n; i++) {
				const alpha = A[i][p] / A[p][p];
				b[i] -= alpha * b[p];
				for (let j = p; j < n; j++) {
					A[i][j] -= alpha * A[p][j];
				}
			}
		}

		// Rückwärtseinsetzen
		const x = new Array(n).fill(0);
		for (let i = n - 1; i >= 0; i--) {
			let sum = 0;
			for (let j = i + 1; j < n; j++) sum += A[i][j] * x[j];
			x[i] = (b[i] - sum) / A[i][i];
		}
		return x;
	}
}