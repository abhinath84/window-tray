import { resolve, dirname } from "path";
import { fileURLToPath } from "url";

import { WindowsTrayIcon } from "../windowstrayicon.js";

const __dirname = dirname(fileURLToPath(import.meta.url));

const options = {
	title: "Tray Demo",
	icon: resolve(__dirname, "../../resource/image/compass-orig.ico")
};

const menu = [
	{
		id: 10,
		title: 'Email',
		cb: () => {
			console.log("clicked send email");
		}
	},
	{
		id: 20,
		title: 'Notify',
		cb: () => {
			console.log("clicked notify");
		}
	},
	{
		id: 30,
		title: '---'
	},
	{
		id: 50,
		title: 'Exit',
		cb: () => {
			console.log("clicked exit");

			tray.destroy();
			process.exit(0);
		}
	}
];

const tray = new WindowsTrayIcon(options);
// tray.start();
tray.setContextMenu(menu);
tray.on("click", () => {
	console.log("================== clicked ===================");
});

setInterval(function () {
	if (tray.isWindowMinimized(options.title) == true)
	{
		tray.toggleWindow(options.title);
		tray.balloon(options.title, 'Will continue in background', 5000);

		return;
	}

	console.log('In background');
}, 1000);
