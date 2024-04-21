path1 = "~/_tmp/sample_out.txt"
path2 = "~/_tmp/sample_output.txt"

def ReadFile(path):
	file = open(path)
	lines = file.readlines()
	transactionSupportMap = {} # <array of itemId>: <support>
	for line in lines:
		line = line.strip()
		itemIdsStr, support = line.split(":")
		itemIds = itemIdsStr.split(",")
		itemIds = [int(n) for n in itemIds]
		itemIds.sort()

		transactionSupportMap[tuple(itemIds)] = support
	return transactionSupportMap

def MissingInTarget(baseMap, targetMap): # "some entry" in baseMap is missing in targetMap
	missingMap = {}
	# print(baseMap)
	for (key, value) in baseMap.items():
		if (not key in targetMap or not value == targetMap[key]):
			missingMap[key] = value
	return missingMap

def DiffSupport(m1, m2):
	diffKeys = {}
	diffMap = {}
	for (key, value) in m1.items():
		if (key in m2 and value != m2[key]):
			diffKeys[key] = 1
	for (key, value) in m2.items():
		if (key in m1 and value != m1[key]):
			diffKeys[key] = 1
	for (key, value) in diffKeys:
		diffMap[key] = (m1[key], m2[key])
	return diffMap

def PrintMissing(missingMap):
	for key, value in missingMap.items():
		string = ""
		i = 0
		for itemIndex in key:
			if (i != 0):
				string += ","
			string += str(itemIndex)
			i += 1
		string += ":" + value
		print(string)
def PrintDiff(diffMap):
	for (key, value) in diffMap.items():
		string = ""
		i = 0
		for itemIndex in key:
			if (i != 0):
				string += ","
			string += str(itemIndex)
			i += 1
		string += "\t" + value[0] + "\t" + value[1]
		print(string)


map1 = ReadFile(path1)
map2 = ReadFile(path2)

missing1 = MissingInTarget(map1, map2)
missing2 = MissingInTarget(map2, map1)

diff = DiffSupport(map1, map2)

print("\n==================\nExists in %s but not in %s" % (path1, path2))
PrintMissing(missing1)
print("\n==================\nExists in %s but not in %s" % (path2, path1))
PrintMissing(missing2)
print("\n==================\nExists in both but different support")
PrintDiff(diff)
