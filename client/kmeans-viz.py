#Importing required modules
import matplotlib.pyplot as plt
import numpy as np
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser('K-means visualizer')
    parser.add_argument('filename', type=str, help='path to the k-means result file')

    args = parser.parse_args()

    #Load Data
    data = np.loadtxt(args.filename, usecols=(0,1), dtype=np.float32)
    #print(data) 

    k = 9 # number of clusters
    #Load the computed cluster numbers
    cluster = np.loadtxt(args.filename, usecols=(2), dtype=np.float32)

    #Plot the results:
    for i in range(k):
        plt.scatter(data[cluster == i , 0] , data[cluster == i , 1] , label = i)
    plt.show()