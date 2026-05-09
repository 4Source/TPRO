export type LedData = {
	index: number;
	col: number;
	row: number;
	mmX: number;
	mmY: number;
	lon: number;
	lat: number;
	continents: ContinentData;
};

export type Coordinates = {
	mmX: number;
	mmY: number;
	lon: number;
	lat: number;
};

export type LedContinentMapping = {
	index: number;
	continents: ContinentData;
};

export enum Continents {
	'North' = 'North',
	'South' = 'South',
	'Europe' = 'Europe',
	'Africa' = 'Africa',
	'Asia' = 'Asia',
	'Australia' = 'Australia',
}

export type ContinentData = Record<Continents, number>;