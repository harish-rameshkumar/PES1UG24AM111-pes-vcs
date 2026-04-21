# PES Version Control System (PES-VCS)

## Student Information

Name: Harish Ramesh Kumar 
USN: PES1UG24AM111
Course: Operating Systems  
GitHub Repository: https://github.com/harish-rameshkumar/PES1UG24AM111-pes-vcs

------------------------------------------------------------

## Project Overview

This project implements a mini Git-like Version Control System in C.

Features:

- SHA-256 object storage
- Blob objects
- Tree snapshots
- Index / staging area
- Commit history
- HEAD references
- Branch references
- Object deduplication

------------------------------------------------------------

## Build Commands

make clean  
make all

------------------------------------------------------------

## Run Commands

./pes init  
./pes add <file>  
./pes status  
./pes commit -m "message"  
./pes log

------------------------------------------------------------

## PHASE 1 — OBJECT STORE

### Screenshot 1A — test_objects Passed

<img width="769" height="136" alt="image" src="https://github.com/user-attachments/assets/ab01a45d-c6a2-414a-99e4-6402e5892cab" />


### Screenshot 1B — .pes/objects Created
<img width="654" height="102" alt="os-ut-orange-problemS find pesobjects -type f" src="https://github.com/user-attachments/assets/b03ea6ba-51d7-4b20-9c87-a92f790a009b" />



------------------------------------------------------------

## PHASE 2 — TREE OBJECTS

### Screenshot 2A — test_tree Passed


<img width="624" height="99" alt="os-u4-orange-problem S  test_tree" src="https://github.com/user-attachments/assets/b9e22fd4-2397-4d44-bcb2-1d16e100d270" />




### Screenshot 2B — Raw Object Data


<img width="869" height="127" alt="es-uf-oronoe-problemS find pesobjects" src="https://github.com/user-attachments/assets/bf19fc01-d1bf-496d-b76e-ee7f2140a615" />




------------------------------------------------------------

## PHASE 3 — INDEX / STAGING

### Screenshot 3A — pes status Output


<img width="879" height="364" alt="-probles 5 pes stotus" src="https://github.com/user-attachments/assets/29c92bb8-4c5c-41d9-a78c-5f4eff87a013" />



------------------------------------------------------------

## PHASE 4 — COMMITS / HISTORY

### Screenshot 4A — pes log Output


<img width="810" height="562" alt="Initial comit" src="https://github.com/user-attachments/assets/c0bb75d7-1af0-4d46-8a28-b1ca6cba9d77" />




### Screenshot 4B — Final Object Store


<img width="688" height="239" alt="os-us-orange-problem s find  pes type fl sort" src="https://github.com/user-attachments/assets/9d3fb9eb-19dd-4c47-9e5e-e41145310933" />




### Screenshot 4C — HEAD and refs/heads/main


<img width="577" height="91" alt="os-u4-orange-problen S cat  pesrefsheadsmain" src="https://github.com/user-attachments/assets/c33c1133-94f0-4971-beae-1067be3f33ae" />




------------------------------------------------------------

## FINAL INTEGRATION TEST

### make test-integration Output


<img width="776" height="541" alt="os-ut-oronge-problen S make test-integration" src="https://github.com/user-attachments/assets/af00304c-7069-433e-81f6-0c90f9f3b5c3" />


<img width="798" height="579" alt="e101coti8o2588e61fo8544b423c05886715Sde8c40670874209237386" src="https://github.com/user-attachments/assets/f54317f9-7aad-42bb-b9d4-89e02206ac50" />

<img width="698" height="212" alt="pesoblectstbd69038bd909cC59340610ab65do429b525561147faa7b5b922919c9023143c" src="https://github.com/user-attachments/assets/3666415d-e8cd-47db-b4e3-da91c438f29d" />



------------------------------------------------------------

## Summary

Successfully implemented a lightweight Git-inspired Version Control System in C using:

- SHA-256 hashing
- Object storage
- Tree snapshots
- Index staging
- Commit history
- Branch references

All phases completed successfully and integration tests passed.
