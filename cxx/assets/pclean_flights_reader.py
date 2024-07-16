import pandas
# Short script for reading in the flight data and converting them in to an HIRM compatible format.

def main():
  df = pandas.read_csv('flights_clean.csv')

  f = open('pclean-flights-clean.obs', 'w')

  # Write out value, name of field, record # for HIRM.
  for index, row in df.iterrows():
    for col_name in df.columns:
      if col_name == 'tuple_id':
        continue
      value = row[col_name]
      # NOTE: HIRM expects space separated data, hence we hyphenate.
      new_col_name = col_name.replace(' ', '-')
      if isinstance(value, str):
        new_value = value.replace(' ','-')
      else:
        new_value = value
      f.write(f"{new_value} {new_col_name} {row['tuple_id']}\n")
  f.close()

main()