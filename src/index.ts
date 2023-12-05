import { EventEmitter } from 'events';
import { resolve, dirname } from "path";
import { fileURLToPath } from "url";
import { inherits } from 'util';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const { NodeTray } = require('./tray.node');

const __dirname = dirname(fileURLToPath(import.meta.url));

export type TrayMenu = {
  id: number,
  title: string,
  cb?: () => void
};

// const { NodeTray } = bindings("tray.node");
inherits(NodeTray, EventEmitter);

export class WindowsTrayIcon extends EventEmitter {
  #tray: typeof NodeTray;
  #menu: TrayMenu[];
  
  constructor(options: {title: string, icon: string}) {
    super();

    this.#menu = [];
    this.#tray = new NodeTray(options.icon);
    this.#setup(options);
  }

  setContextMenu(menu: TrayMenu[]) {
    this.#menu = menu;
  }

  isWindowMinimized(title: string) {
    return (this.#tray.isWindowMinimized(title));
  }

  toggleWindow(title: string) {
    return (this.#tray.toggleWindow(title));
  }

  balloon(title: string, message: string, timeout: number) {
    return (this.#tray.balloon(title, message, timeout));
  }

  // start() {
  //   return (this.#tray.start());
  // }

  // stop() {
  //   return (this.#tray.stop());
  // }

  destroy() {
    return (this.#tray.destroy());
  }

  #setup(options: {title: string, icon: string}) {
    this.#tray.setToolTip(options.title);
    this.#tray.on('click', () => {
      let result = this.#tray.toggleWindow(options.title);
      console.log("click, result = ", result);

      this.emit("click");
    });
    this.#tray.on('right-click', () => {
      const menu = this.#menu.map((item: TrayMenu) => {
        return ({ id: item.id, title: item.title });
      });
    
      this.#tray.showPopup(menu, (err: Error, result: number) => {
    
        console.log('error:', err);
        console.log('result:', result);

        const found = this.#menu.find((item) => item.id === result);
        if (found && found.cb) found.cb();
      });
    });

    this.#tray.on('balloon-click', () => {
      console.log('balloon-click')
    })
  }

  #shutdown(): void {
    console.log('Shutdown!');
    this.#tray.destroy();
    process.exit(0);
  }
}
