import { useState, useEffect } from 'preact/hooks';

export function Clock() {
	const [time, setTime] = useState(Date.now());

	useEffect(() => {
		const timer = setInterval(() => setTime(Date.now()), 1000);
		return () => clearInterval(timer);
	}, []);

	return <span>{new Date(time).toLocaleTimeString()}</span>;
}
