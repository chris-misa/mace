import numpy as np
import sys
average = 1/float(sys.argv[1])
exponential = np.random.exponential(average, 1000)
print(exponential)
