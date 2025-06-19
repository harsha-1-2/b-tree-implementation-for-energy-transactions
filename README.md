# b-tree-implementation-for-energy-transactions
# Energy Trading Record Management System

This C-based project implements a record management system for energy trading transactions between buyers and sellers. It utilizes B+ Trees for organizing and storing data related to transactions, sellers, and buyers efficiently.

## Overview

The system maintains the following entities:

- **Transactions**: Each transaction includes buyer and seller IDs, energy amount in kWh, price per kWh, and timestamp.
- **Sellers**: Each seller has unique pricing for energy below and above 300 kWh, a total revenue tracker, and a set of regular buyers.
- **Buyers**: Tracks total energy purchased and maintains a tree of their transactions.
- **Seller-Buyer Pairs**: Tracks the number of transactions between each seller and buyer.

B+ Trees are used for efficient insertion, searching, and sorted traversal of data.

## Features

- Add new energy transactions
- Display all transactions with full details
- Generate transaction sets for every seller or buyer
- Filter transactions based on a given time range
- Calculate total revenue for a specific seller
- Display transactions within a given energy range in sorted order
- Sort and display buyers by total energy purchased
- Sort seller-buyer pairs by transaction frequency
- Import/export transaction data from/to a file (`transactions.txt`)

## Data Structures Used

- **B+ Trees**: For indexing and searching records (buyers, sellers, transactions).
- **Linked Lists**: Used to track regular buyers (buyers with ≥5 transactions with the same seller).
- **Structs**: Used for entities like Buyer, Seller, Transaction, and SellerBuyerPair.

## How to Compile and Run

```bash
gcc new_final.c -o energy_trading_system
./energy_trading_system
