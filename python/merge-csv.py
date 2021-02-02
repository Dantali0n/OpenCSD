import pandas as pd

a = pd.read_csv("dvb-t-i.csv")
b = pd.read_csv("dvb-t-q.csv")
b = b.dropna(axis=1)
merged = a.merge(b, on='index')
merged.to_csv("dvb-t.csv", index=False)