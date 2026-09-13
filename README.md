# High-Frequency Currency Arbitrage Engine

Implementing foreign currency exchange arbitrage, a form of high-frequency trading. Forex arbitrage is the strategy of exploiting price disparity in the forex markets. It may be effected in various ways but however it is carried out, the arbitrage seeks to buy currency prices and sell currency prices that are currently divergent but extremely likely to rapidly converge. The expectation is that as prices move back towards a mean, the arbitrage becomes more profitable and can be closed, sometimes even in milliseconds.

## Data

All historical Forex data is sourced from [Dukascopy Bank](https://www.dukascopy.com/swiss/english/marketwatch/historical/).

Stored at `\data`, the .csv files are separated into `\bid` and `\ask` subdirectories, respectively containing bid and ask rates for each given currency pair, over the duration of one day (3/26/2025).

The timestamps in the first column iterate roughly each second, starting at 00:00:00 GMT. The exact number of rows/iterations differs between currency pairs. However, for a given currency pair, the bid and ask data contain the same number of iterations.

The volume in the fourth column is measured in thousands.

Here is the list of all available currency pairs, sorted alphabetically, in the same order as their respective files:

- AUD/CAD
- AUD/JPY
- AUD/SGD
- AUD/USD
- CAD/JPY
- EUR/AUD
- EUR/CAD
- EUR/GBP
- EUR/JPY
- EUR/SGD
- EUR/USD
- GBP/AUD
- GBP/CAD
- GBP/JPY
- GBP/USD
- SGD/JPY
- USD/CAD
- USD/JPY
- USD/SGD
