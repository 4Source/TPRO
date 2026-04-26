import { useState } from 'preact/hooks';

export function useZoomPan() {
	const [zoom, setZoom] = useState(1);
	const [pan, setPan] = useState({ x: 0, y: 0 });
	const [isDragging, setIsDragging] = useState(false);
	const [dragStart, setDragStart] = useState({ x: 0, y: 0 });

	const handleWheel = (e: WheelEvent) => {
		e.preventDefault();
		const wrapper = e.currentTarget as HTMLElement;
		const rect = wrapper.getBoundingClientRect();
		const zoomSpeed = 0.1;
		const newZoom = Math.max(1, zoom - (e.deltaY > 0 ? zoomSpeed : -zoomSpeed));

		// 20px offset aligns zoom origin to the matrix content area (inside padding)
		const rawX = e.clientX - rect.left - 20;
		const rawY = e.clientY - rect.top - 20;
		const logicalX = (rawX - pan.x) / zoom;
		const logicalY = (rawY - pan.y) / zoom;
		const zoomDiff = newZoom - zoom;
		setPan({ x: pan.x - (logicalX * zoomDiff), y: pan.y - (logicalY * zoomDiff) });
		setZoom(newZoom);
	};

	const handleMouseDown = (e: MouseEvent) => {
		setIsDragging(true);
		setDragStart({ x: e.clientX - pan.x, y: e.clientY - pan.y });
	};

	const handleMouseMove = (e: MouseEvent) => {
		if (!isDragging) return;
		setPan({ x: e.clientX - dragStart.x, y: e.clientY - dragStart.y });
	};

	const handleMouseUp = () => setIsDragging(false);

	return { zoom, pan, isDragging, handleWheel, handleMouseDown, handleMouseMove, handleMouseUp };
}
