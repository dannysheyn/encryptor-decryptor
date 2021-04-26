# encryptor-decryptor
Encryptor decryptor using posix message queue
Note:
This exe runs only on a linux system. (because of use of posix message queue)

Instructions:
Inorder for the encryption to work you need to install 'openssl' with 'sudo apt-get install libssl-dev'.
Run 'make' and compile the executable.
Run './server.out', and then run the decryptors with:
'./decryptor.out' with the decryptor id and the optional wanted flag (-n) that indicates the number of times to decrypt a password.
examples:
./decryptor.out 1 -n 2 
./decryptor.out 5 
OR:
Run './launcher.out', with the the number of decryptors and the flag of '-n' to indicate the number of times a decryptor decrypts a password.
examples:
./launcher.out 2 -n 5 # run 2 decryptors that will decrypt 5 times each.
./launcher.out 2 # run 2 decryptors that will decrypt forever.
