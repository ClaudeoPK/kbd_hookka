![Claudy](https://github.com/ClaudeoPK/kbd_hookka/blob/main/show.gif?raw=true)
## 0xFFFFFFFF kbd_hookka
**This PoC shows one of the simplest way to prevent HID input manipulation**

## 0x0 EntryPoint
  A few days ago I heard some poor RPG game is suffering from macro users and so on finally found the answer on the toilet.
  Most of them use Kernel-Based libraries to send HID input to Operating System.
  There are some filter drivers and more cheat-like drivers but this PoC aims them and it is good enough unless KPP(aka PatchGuard) turns his eyeball to kbdclass.sys in   future.

## 0x1 Process
  Find KeybardClassServiceCallback by scanning AOB pattern.
  Place inline hook to entry of KeyboardClassServiceCallback.

## 0x2 ToDO
  Add getDriverObject(PVOID ReturnAddress)
