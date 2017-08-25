# Craft-L
一个使用NCurses的开源Minecraft复刻。  
An Opensource Minecraft 3D Clone uses NCurses Library.  
![](/2017-08-25-143402_654x390_scrot.png)
  
# About  
- 这是一个Minecraft和Dwarf Fortress的3D结合，与其他所有的复刻版不同的是——它是运行在终端的。它采用类似Dwarf Fortress的3D显示模式，虽然表面上看上去是2D的，但是通过各个切面视角的转换，你能够清楚的想象出所处的3D环境。  
This is a 3D combination of the Minecraft and the Dwarf Fortress.But it runs in the terminal.It's 3D display mode is like the Dwarf Fortress.It looks like a 2D game.But through many perspectives,you can mention where you are standing.  
  
- 这个项目的最终目标是实现Minecraft和Dwarf Fortress的完美结合，并且不使用任何图形库。由于画面不直观的原因，我们会尽可能地提高游戏性和可玩性，增加更为令人激动的游戏机制。目前，这个项目正处于开发初期。  
The target of this project is to make a combination of the Minecraft and the Dwarf Fortress without any Graphical Library.We will try to improve the gameplay and playability as we can,because the graphical is not very directly.The project is starting just a while.  
  
# Usage
- 使用```./craft-l```命令即可启动Craft-L。  
Use ```./craft-l``` command to start Craft-L.  
  
  
```X:287 Y:145 Z:-41 row:24 col:80 D-M:1 N:* S(1):1 * ```  
- ^  状态栏的内容：  
The informations from the status area:  
- ```X```：表示你所处在的行位置。  
```X``` : Which row you are now standing.  
- ```Y```：表示你所处在的列位置。  
```Y``` : Which col you are now standing.  
- ```Z```：表示你所处在的高度（0表示地平线，负数为地下，正数为地上）。  
```Z``` : Which height you are now standing (0 for horizon,A negative number for the underground,A positive number for the ground).  
- ```row```：您当前窗口的行数。  
```row``` : The number of your window's row.  
- ```col```：您当前窗口的列数。  
```col``` : The number of your window's col.  
- ```D-M```：显示模式。1表示X-Y切面，2表示Y-Z切面，3表示X-Z切面。这三个显示模式的存在使得你能清楚的知道你处在3D世界的哪一个角落。  
```D-M``` : Display Mode.1 for X-Y perspective,2 for Y-Z perspective,3 for X-Z perspective.This 3 display modes make you can know clearly about where you are standing now.  
- ```N```：英文现在（now）的首字母。意为现在所拿的物品。  
```N``` : The first letter of 'now'.Means which stuff on your hand now.  
- ```S(number)```：英文物品（stuff）的首字母。意味你所携带的物品。  
```S(number)``` : The first letter of 'stuff'.Means all the stuffs you have got now.  

# License
This project is using GPL-3.0+  
本项目使用GPL-3.0或更高版本协议。  
