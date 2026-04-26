import './style.css';

type Props = {
	href: string;
	title: string;
	description: string;
};

export function Resource({ href, title, description }: Props) {
	return (
		<a href={href} target="_blank" class="resource" rel="noreferrer">
			<h2>{title}</h2>
			<p>{description}</p>
		</a>
	);
}
