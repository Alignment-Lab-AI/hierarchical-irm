# Generates a mixture of 2 normals as an HIRM observation file.
import numpy as np

NUM_SAMPLES = 1000

def main():
  rng = np.random.default_rng(1762)
  names = ['a', 'bbbbbbb', 'cbcbcbcbcbcbcbcb', 'zzz', 'wwwffwwwffwwwff']
  samples = np.random.choice(names, NUM_SAMPLES, p=[0.2, 0.2, 0.4, 0.15, 0.05])
  f = open('test_bigram.obs', 'w')
  for i in range(NUM_SAMPLES):
    f.write(f"{samples[i]},has_value,{i}_{samples[i][0]}\n")
  f.close()

main()
