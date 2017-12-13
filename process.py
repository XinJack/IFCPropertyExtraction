'''
	对ifcviewer输出的文件进行处理，从而提取出所有的属性
'''
import os 
import argparse

# 命令行工具，添加input属性
parser = argparse.ArgumentParser()

parser.add_argument('input', help="输入文件路径，如果有空格使用双引号包裹")
args = parser.parse_args()

input_file = args.input

# 确保输入的文件名存在
if(input_file.startswith("'") and input_file.endswith("'")):
	input_file = input_file[1: -1]
if(not os.path.abspath(input_file)):
	input_file = os.path.join(os.getcwd(), input_file)
	print(input_file)
if(not os.path.exists(input_file)):
	print('文件{0}不存在'.format(input_file))
else:
	# 构造输出文件路径
	output1 = input_file[0: -4] + '-attributes.txt'
	output2 = input_file[0: -4] + '-prolog.pro'

	# 字典，用于存储解析出来的所有属性
	attr_dict = dict()

	with open(input_file, 'r')  as f:
		line = f.readline().strip()
		while(line != ''):
			# 获取头信息
			if(line.startswith('Header')):
				attr_dict['Header Info'] = dict()
				line = f.readline().strip()
				while(not line.startswith('Ifc')): #顺序读取文件知道line以ifc开头，说明该阶段读取结束
					if line.find('=') != -1:
						tmp = line.split('=')
						attr_name = tmp[0].strip()
						attr_value = tmp[1].strip()[1: -1] # 去掉原本自带的引号
						attr_dict['Header Info'][attr_name] = attr_value
					line = f.readline().strip()
			# 获取ifc实体信息
			if(line.startswith('Ifc')):
				tmp = line.split(" '") # 有点tricky的属性获取方法
				ifc_type = tmp[0]
				guid = tmp[1][0: -1]
				name = tmp[2][0: -1] if len(tmp) > 2 else ""
				attr_dict[guid] = dict() # 存放guid的ifc实体属性的字典
				attr_dict[guid]['IFC类型'] = ifc_type
				attr_dict[guid]['GUID'] = guid
				attr_dict[guid]['NAME'] = name
				line = f.readline().strip()
				while(not line.startswith('Ifc')): # 顺序读取直到到下一个ifc实体
					if(line.find('=') != -1):
						tmp = line.split('=')
						attr_name = tmp[0].strip()
						attr_value = tmp[1].strip()
						attr_dict[guid][attr_name] = attr_value
					line = f.readline()
					if(line == ''): # 确保在文件结束的时候能够正常退出
						break
					else:
						line = line.strip()

	# # 将提取出来的属性进行格式化输出
	with open(output1, 'w', encoding='utf-8') as f1, open(output2, 'w', encoding='utf-8') as f2:
		for guid in attr_dict:
			f1.write(guid + '\n')
			f1.write('{' + '\n')
			# print(guid)
			# print('{')
			for attr_name in attr_dict[guid]:
				attr_value = attr_dict[guid][attr_name]
				attr_name = attr_name.replace(' ', '')
				attr_value = attr_value.replace('\'', '')
				# 输出prolog逻辑事实
				# print("'{0}'('{1}', '{2}')".format(attr_name, guid, attr_dict[guid][attr_name]))
				# print("\t'{0}' : '{1}'".format(attr_name, attr_dict[guid][attr_name]))
			# print('}')
				if attr_name == '编号': # 编号不需要转成数值类型
					f2.write("'{0}'('{1}', '{2}').\n".format(attr_name, guid, attr_value))
					continue
				try:
					attr_value = float(attr_value)
					f2.write("'{0}'('{1}', {2}).\n".format(attr_name, guid, attr_value))
				except:
					f2.write("'{0}'('{1}', '{2}').\n".format(attr_name, guid, attr_value))
				f1.write("\t'{0}' : '{1}', \n".format(attr_name, attr_value))
			f1.write('}' + '\n')
	print('属性提取完成')