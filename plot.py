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

tnrPerThreshold = {}
fnrPerThreshold = {}
avgTnrPerThresold = []
avgFnrPerThresold = []

tpPerThreshold = {}
tnPerThreshold = {}
fpPerThreshold = {}
fnPerThreshold = {}
avgPrecisionPerThresold = []
avgRecallPerThresold = []
avgFpPerThresold = []
avgFnPerThresold = []
avgTpPerThresold = []
avgTnPerThresold = []


userCounts = []
averageDetectorRuntimes = []
averageDetectorRuntimesPerUserCount = {}

percentAgeOfDetections = {}

avgProbesPerThresholdDict = {}
avgProbesPerThreshold = []

def parseRocTextFile():
	global tpPerThreshold
	global fpPerThreshold
	global fnPerThreshold
	global avgPrecisionPerThresold
	global avgRecallPerThresold

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

		tnr = tn / (tn + fp)
		fnr = fn / (fn + tp)

		if threshold not in tprPerThreshold.keys():
			tprPerThreshold[threshold] = [tpr]
			fprPerThreshold[threshold] = [fpr]

			tnrPerThreshold[threshold] = [tnr]
			fnrPerThreshold[threshold] = [fnr]

			tpPerThreshold[threshold] = [tp]
			tnPerThreshold[threshold] = [tn]
			fpPerThreshold[threshold] = [fp]
			fnPerThreshold[threshold] = [fn]
		else:
			tprPerThreshold[threshold].append(tpr)
			fprPerThreshold[threshold].append(fpr)

			tnrPerThreshold[threshold].append(tnr)
			fnrPerThreshold[threshold].append(fnr)

			tpPerThreshold[threshold].append(tp)
			tnPerThreshold[threshold].append(tn)
			fpPerThreshold[threshold].append(fp)
			fnPerThreshold[threshold].append(tn)

	global thresholds
	thresholds = list(tprPerThreshold.keys())
	thresholds.sort()
	# for k in tprPerThreshold.keys():
	for k in thresholds:
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


		total = 0
		count = 0
		for tnr in tnrPerThreshold[k]:
			count += 1
			total += tnr
		avgTnr = total / count

		count = 0
		total = 0
		for fnr in fnrPerThreshold[k]:
			count += 1
			total += fnr
		avgFnr = total / count

		avgTnrPerThresold.append(avgTnr)
		avgFnrPerThresold.append(avgFnr)

		count = 0
		total = 0
		for tn in tnPerThreshold[k]:
			count += 1
			total += tn
		avgTn = total / count
		avgTnPerThresold.append(avgTn)
		count = 0
		total = 0
		for tp in tpPerThreshold[k]:
			count += 1
			total += tp
		avgTp = total / count
		avgTpPerThresold.append(avgTp)
		count = 0
		total = 0
		for fp in fpPerThreshold[k]:
			count += 1
			total += fp
		avgFp = total / count
		avgFpPerThresold.append(avgFp)
		count = 0
		total = 0
		for fn in fnPerThreshold[k]:
			count += 1
			total += fn
		avgFn = total / count
		avgFnPerThresold.append(avgFn)
		if (avgTp + avgFp == 0):
			avgPrec = 0
		else:
			avgPrec = avgTp / (avgTp + avgFp)
		if (avgTp + avgFn == 0):
			avgRecall = 0
		else:
			avgRecall = avgTp / (avgTp + avgFn)
		avgPrecisionPerThresold.append(avgPrec)
		avgRecallPerThresold.append(avgRecall)
	print(thresholds)
	print()
	print("Average TPR per threshold")
	print(avgTprPerThresold)
	print("Average FPR per threshold")
	print(avgFprPerThresold)
	print()
	print("Average TP per threshold")
	print(avgTpPerThresold)
	print("Average TN per threshold")		
	print(avgTnPerThresold)
	print("Average FP per threshold")
	print(avgFpPerThresold)
	print("Average FN per threshold")
	print(avgFnPerThresold)
	print()
	print("Average Precision per threshold")
	print(avgPrecisionPerThresold)
	print("Average Recall per threshold")
	print(avgRecallPerThresold)


def plotRoc():
	parseRocTextFile()
	f, ax = plt.subplots(1)
	ax.set_ylim(0, 1)
	ax.set_xlim(0, 1)
	ax.plot([0, 1], [0, 1], transform=ax.transAxes, ls="--")
	ax.plot(avgFprPerThresold, avgTprPerThresold)
	ax.set_xlabel("False Positive Rate")
	ax.set_ylabel("True Positive Rate")
	# ax.set_title("Receiver Operating Characteristic (ROC) Curve")
	plt.show()
	plt.savefig("roc.png")

	f, ax = plt.subplots(1)
	ax.set_ylim(0, 1)
	ax.set_xlim(0, 1)
	ax.plot([0, 1], [0, 1], transform=ax.transAxes, ls="--")
	ax.plot(avgFnrPerThresold, avgTnrPerThresold)
	ax.set_xlabel("False Negative Rate")
	ax.set_ylabel("True Negative Rate")
	# ax.set_title("Receiver Operating Characteristic (ROC) Curve")
	plt.show()
	plt.savefig("roc2.png")

def plotPrecisionRecall():
	f, ax = plt.subplots(1)
	# ax.set_ylim(0, 1)
	# ax.set_xlim(0, 1)
	# ax.plot([0, 1], [0, 1], transform=ax.transAxes, ls="--")
	ax.plot(avgRecallPerThresold, avgPrecisionPerThresold)
	ax.set_xlabel("Recall")
	ax.set_ylabel("Precision")
	ax.set_title("Precision Recall Curve")
	plt.show()
	plt.savefig("precisionRecall.png")

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
	# ax.set_title("Average Micoseconds Per Detector Update\nfor Different User Counts (3500 inital bridges)")
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
	# ax.set_title("Average Probe Accesses Per Detector Update\nfor Different Classification  Thresholds")
	plt.show()
	plt.savefig("avgProbes.png")

def main():	
	plotRoc()
	plotPrecisionRecall()	
	plotAverageDetectorRuntime()
	plotAvgProbesVsThreshold()
	

if __name__ == "__main__":		
	main()	