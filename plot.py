import pandas
import matplotlib.pyplot as plt
from math import log10, floor


thresholds = []
truePosRates = []
falsePosRates = []

tprPerThreshold = {}
fprPerThreshold = {}
avgTprPerThresold = []
avgFprPerThresold = []

userCounts = []
averageDetectorRuntimes = []
averageDetectorRuntimesPerUserCount = {}

percentAgeOfDetections = {}

avgProbesPerThresholdDict = {}
avgProbesPerThreshold = []

def parseRocTextFile():
	rocTxtFile = open('roc.txt', 'r')
	
	while True:
		line = rocTxtFile.readline()

		if not line:
			break

		threshold = float(line.split("=")[1])
		tp = int(rocTxtFile.readline().split("=")[1])
		tn = int(rocTxtFile.readline().split("=")[1])
		corrects = int(rocTxtFile.readline().split("=")[1])
		fp = int(rocTxtFile.readline().split("=")[1])
		fn = int(rocTxtFile.readline().split("=")[1])
		incorrects = int(rocTxtFile.readline().split("=")[1])

		tpr = tp / (tp + fn)
		fpr = fp / (fp + tn)

		if threshold not in tprPerThreshold.keys():
			tprPerThreshold[threshold] = [tpr]
			fprPerThreshold[threshold] = [fpr]
		else:
			tprPerThreshold[threshold].append(tpr)
			fprPerThreshold[threshold].append(fpr)

	global thresholds
	thresholds = list(tprPerThreshold.keys())
	thresholds.sort()
	for k in tprPerThreshold.keys():
		print("threshold = " + str(k))
		# print(tprPerThreshold[k])
		# print(fprPerThreshold[k])
		total = 0
		count = 0
		for tpr in tprPerThreshold[k]:
			count += 1
			total += tpr

		avgTpr = total / count

		count = 0
		total = 0
		for fpr in fprPerThreshold[k]:
			count += 1
			total += fpr

		avgFpr = total / count

		avgTprPerThresold.append(avgTpr)
		avgFprPerThresold.append(avgFpr)
	print(avgTprPerThresold)
	print(avgFprPerThresold)


def plotRoc():
	parseRocTextFile()
	f, ax = plt.subplots(1)
	ax.set_ylim(0, 1)
	ax.set_xlim(0, 1)
	ax.plot([0, 1], [0, 1], transform=ax.transAxes, ls="--")
	ax.plot(avgFprPerThresold, avgTprPerThresold)
	ax.set_xlabel("False Positive Rate")
	ax.set_ylabel("True Positive Rate")
	ax.set_title("Receiver Operating Characteristic (ROC) Curve")
	plt.show()
	plt.savefig("roc.png")

def parseAverageTimeFile():
	file = open('averageTime.txt', 'r')
	
	while True:
		line = file.readline()

		if not line:
			break

		userCount = int(line.split("=")[1])
		runtime = float(file.readline().split("=")[1])		
		
		if userCount not in averageDetectorRuntimesPerUserCount.keys():
			averageDetectorRuntimesPerUserCount[userCount] = [runtime]
		else:
			averageDetectorRuntimesPerUserCount[userCount].append(runtime)

	global userCounts
	userCounts = list(averageDetectorRuntimesPerUserCount.keys())	
	userCounts.sort()	
	for k in userCounts:
		total = 0
		count = 0
		for r in averageDetectorRuntimesPerUserCount[k]:
			count += 1
			total += r

		avg = total / count
		averageDetectorRuntimes.append(avg)

def format_func(value, tick_number=None):
    num_thousands = 0 if abs(value) < 1000 else floor (log10(abs(value))/3)
    value = round(value / 1000**num_thousands, 2)
    return f'{value:g}'+' KMGTPEZY'[num_thousands]

def nanoToMicro(value, tick_number=None):
    num_thousands = 0 if abs(value) < 1000 else floor (log10(abs(value))/3)
    value = round(value / 1000**num_thousands, 2)
    return value

def plotAverageDetectorRuntime():
	parseAverageTimeFile()
	f, ax = plt.subplots(1)		
	ax.set_xticks(userCounts)
	ax.xaxis.set_major_formatter(plt.FuncFormatter(format_func))
	ax.yaxis.set_major_formatter(plt.FuncFormatter(nanoToMicro))
	ax.plot(userCounts, averageDetectorRuntimes)
	ax.set_xlabel("User Count")
	ax.set_ylabel("Avg μs / Detector Update")
	ax.set_title("Average Micoseconds Per Detector Update\nfor Different User Counts (3500 inital bridges)")
	plt.show()
	plt.savefig("avgRuntime.png")

# def detectionVsBlockedFile():
# 	file = open('detectionVsBlocked.txt', 'r')
# 	while True:
# 		line = file.readline()
# 		if not line:
# 			break
# 		threshold = float(line.split("=")[1])
# 		runtime = float(file.readline().split("=")[1])			
# 		if threshold not in percentAgeOfDetections.keys():
# 			percentAgeOfDetections[threshold] = [runtime]
# 		else:
# 			percentAgeOfDetections[threshold].append(runtime)

def parseAverageProbesVsThresholdFile():
	file = open('probesVsThreshold.txt', 'r')
	
	while True:
		line = file.readline()

		if not line:
			break

		threshold = float(line.split("=")[1])
		total_probs = int(file.readline().split("=")[1]) 
		avg_probes = int(float(file.readline().split("=")[1]))
		

		if threshold not in avgProbesPerThresholdDict.keys():
			avgProbesPerThresholdDict[threshold] = [avg_probes]			
		else:
			avgProbesPerThresholdDict[threshold].append(avg_probes)			

	thresholds = list(tprPerThreshold.keys())
	thresholds.sort()
	for k in thresholds:
		total = 0
		count = 0
		for tpr in avgProbesPerThresholdDict[k]:
			count += 1
			total += tpr

		avg = total / count

		avgProbesPerThreshold.append(avg)

def plotAvgProbesVsThreshold():
	parseAverageProbesVsThresholdFile()
	f, ax = plt.subplots(1)			
	ax.plot(thresholds, avgProbesPerThreshold)
	ax.set_xlabel("Classification Threshold (Minimum Confidence to Probe)")
	ax.set_ylabel("Avg Probe Accesses / Detector Update")
	ax.set_title("Average Probe Accesses Per Detector Update\nfor Different Classification  Thresholds")
	plt.show()
	plt.savefig("avgProbes.png")

def main():
	plotRoc()
	plotAverageDetectorRuntime()
	plotAvgProbesVsThreshold()

if __name__ == "__main__":		
	main()	