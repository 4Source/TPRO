
interface LED { x: number; y: number; lat: number; lon: number; index: number; }

/**
 * Hauptfunktion zum Generieren und Hochladen der Sonnendaten
 * @param leds Deine LED-Liste aus der led-config.json
 * @param year Das Zieljahr (z.B. 2024 für Schaltjahr)
 * @param onProgress Callback-Funktion für Status-Updates im UI
 */
export async function startSunDataGeneration(
    leds: LED[],
    year: number,
    onProgress: (status: string, progress: string) => void
) {
    const isLeapYear = (year % 4 === 0 && year % 100 !== 0) || (year % 400 === 0);
    const daysInMonth = [31, isLeapYear ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];

    const fetchWithRetry = async (url: string, retries = 5): Promise<any> => {
        try {
            const res = await fetch(url);
            if (res.status === 429) {
                onProgress("Rate Limit!", "Warte 10 Sekunden...");
                await new Promise(r => setTimeout(r, 10000));
                return fetchWithRetry(url, retries - 1);
            }
            return await res.json();
        } catch (e) {
            if (retries > 0) {
                await new Promise(r => setTimeout(r, 2000));
                return fetchWithRetry(url, retries - 1);
            }
            throw e;
        }
    };

    const uploadToEsp = async (dateStr: string, data: any, retries = 5) => {
        try {
            const res = await fetch(`/file/sun-data/${dateStr}.json`, {
                method: 'PUT',
                body: JSON.stringify(data),
            });
            if (res.status !== 201) {
                await new Promise(r => setTimeout(r, 5000));
                return uploadToEsp(dateStr, data, retries - 1);
            }
        } catch (e) {
            console.error(`Upload fehlgeschlagen für ${dateStr}`, e);
        }
    };

    const existingOnEsp = async (dateStr: string): Promise<boolean> => {
        try {
            const res = await fetch(`/file/sun-data/${dateStr}.json`, { method: 'GET' });
            return res.status === 200;
        } catch (e) {
            return false;
        }
    };

    // Durch das Jahr loopen
    for (let month = 1; month <= 12; month++) {
        for (let day = 1; day <= daysInMonth[month - 1]; day++) {
            const dateStr = `${year}-${month.toString().padStart(2, '0')}-${day.toString().padStart(2, '0')}`;
            const dailyGrid: Record<string, Record<string, any>> = {};

            onProgress(`Verarbeite Tag: ${dateStr}`, "Starte LEDs...");

            if (await existingOnEsp(dateStr)) {
                onProgress(`Tag ${dateStr} übersprungen`, "Daten bereits vorhanden.");
                continue;
            }

            let count = 0;
            for (const led of leds) {
                count++;
                if (count % 10 === 0) { // Update alle 10 LEDs für Performance
                    onProgress(`Tag ${dateStr}`, `LED ${count} von ${leds.length}`);
                }

                if (led.lat < -90 || led.lat > 90 || led.lon < -180 || led.lon > 180) continue;

                const url = `https://api.sunrise-sunset.org/json?lat=${led.lat.toFixed(4)}&lng=${led.lon.toFixed(4)}&date=${dateStr}&tzid=Europe/Berlin&formatted=0`;

                try {
                    const data = await fetchWithRetry(url);
                    if (data.status === "OK") {
                        const sr = data.results.sunrise.split('T')[1].split('+')[0];
                        const ss = data.results.sunset.split('T')[1].split('+')[0];

                        if (!dailyGrid[led.x]) dailyGrid[led.x] = {};
                        dailyGrid[led.x][led.y] = { sr, ss };
                    }
                    await new Promise(r => setTimeout(r, 100)); // Basis-Delay gegen Ban
                } catch (e) {
                    console.error(`Dauerhafter Fehler am ${dateStr} bei LED ${led.index}`);
                }
            }

            onProgress(`Tag ${dateStr} fertig`, "Sende zum ESP...");
            await uploadToEsp(dateStr, dailyGrid);
        }
    }
    onProgress("Abgeschlossen", "Alle Daten sind auf der SD-Karte.");
}
