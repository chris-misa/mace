import numpy as np
import sys
average = float(sys.argv[1])
s = np.random.poisson(average, 1000)
print(s)
