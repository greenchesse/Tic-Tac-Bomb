# Tic Tac Bomb

> A strategic twist on the classic Tic Tac Toe — where every move counts and the pressure never stops.
> Terminal Base Game.
 
---
 
## Overview
 
**Tic Tac Bomb** keeps the familiar 3×3 grid win condition but introduces a **move expiration system** and a set of powerful **abilities** that add a layer of strategy and tension to every turn.
 
---
 
## How It Works
 
Players earn **Move Points** by placing marks on the board. These points can be spent to activate one of four special abilities:
 
| Ability | Effect |
|--------|--------|
| 💣 **Bomb** | Destroys a marked cell on the board |
| 🛡️ **Shield** | Protects your mark from being bombed or stolen |
| 💊 **Heal** | Restores an expired or destroyed mark |
| 🎯 **Steal** | Converts an opponent's mark into your own |
 
---

## Key Features
 
- Classic Tic Tac Toe win condition (3 in a row)
- Move expiration system — don't sit on your turns
- Resource management through Move Points
- Four unique abilities that change how each match plays out
---
 
## Win Condition

Same as classic Tic Tac Toe — get **3 of your marks in a row** (horizontally, vertically, or diagonally) before your opponent does. Just watch out for bombs. 💣

---
## Git
```Terminal
git clone https://github.com/greenchesse/Tic-Tac-Bomb
```

---
## How to Run
> this only run on linux
### Terminal 1:
```Terminal
~$ cd Tic-Tac-Bomb
~$ gcc Tic-Tac-Bomb-Server.c -o Server
~$ gcc Tic-Tac-Bomb-Client.c -o Client
~$ ./Server [Port Number]
```
### Terminal 2:
```Terminal
~$ cd Tic-Tac-Bomb
~$ ./Client [Port Number] [IP address]
```

## **Enjoy**

