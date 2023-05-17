# pokewalker hax

### Credits
Firstly, credits to [Dmitry Grinberg](https://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker) for a lot of critical information for this project and getting me started. For my own practice and fun, I tried my best to avoid reading his writeup on his hack for the walker as much as I could, but in the end I did cave a bit and made use of Mr. Grinberg's genius to help me kickstart my own project. This would have taken a billion years longer to get working without his precedent. Hats off to you, Dmitry!

## 3ds
The 3ds folder is the in-progress work of essentially porting a user-friendly version of the stm32 application, as a 3DS Homebrew application. It currently doesn't do anything useful besides printing out the raw bytes sent from the Pokewalker.

## stm32
The stm32 folder contains the source of the app developed for the STM32 Nucleo STM32H723ZG ARM development board paired with the [MikroElektronika IrDA 3 click](https://www.mikroe.com/irda-3-click) breakout board through a breadboard. This application is currently in a working state, and is able to gift a user's Pokewalker a free shiny pokemon of their choice, with any held item, level, moveset, and ability they choose.

For those interested in building my code, my development environment for this app was VSCode + Platform.io + STM32CubeMX.
