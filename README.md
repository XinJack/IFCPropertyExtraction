# IFCPROPERTYEXTRACTION
## ifcviewer
1. 项目是基于ifcengine项目提供的ifcviewer例子的c++版本代码实现的
2. 增加的功能
	+ 左侧的目录树在对应的ifc实体文本上添加上guid（后续属性提取需要）
	+ 增加属性提取功能（实现原理目前是从目录树抽取）  
		- 实现功能时修复了源代码的一两个bug，主要是字符串拼写错误导致的 

## process.py
1. 基于argparse的python命令行工具
2. 使用python process.py "[输入文件路径]"
3. 用于将从ifcviewer中导出的属性文件进行再处理，输出两个文件*-attributes.txt和*-prolog.pro两个文件
4. *-attributes.txt文件存放的是json格式的guid和关联的属性
5. *-prolog.pro是给prolog使用的以prolog语法表示的ifc属性信息 

## to-dos
1. 由于从ifcviewer转出的属性值可能是带有单位的，如'275.000000 Milli Metre'，因此需要进行处理，将值和单位分离开来
	+ 想法一：对ifcviewer进行处理
	+ 想法二： 在process.py中进行分情况处理