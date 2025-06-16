Όνομα: Δημήτρης Σέγκεσσερ
A.M.: 5006
=================================================
Project 2 HY-486
Computer Science Department, University Of Crete
=================================================

- Υλοποίηση κατανεμημένου συστήματος βιβλιοθηκών-δανειστών με χρήση MPI σε C.

# Οργάνωση κώδικα #

Έχω μοιράσει την υλοποίηση σε ξεχωριστά modules:

1) client module        - η λειτουργικότητα των πελατών (loaners)
2) library module       - η λειτουργικότητα των βιβλιοθηκών (libraries)
3) coordinator module   - η λειτουργικότητα του συντονιστή (coordinator)
4) utils module         - γενικές δομές όπως message_t και συναρτήσεις για την υλοποίηση

5) main.c               - Αποτελεί το σημείο εισόδου που κανονίζει τους ρόλους των διεργασιών

# Makefile #

- Configuration

Στο Makefile παρέχω τα παρακάτω compile-time options:

1) ALL_DEBUG       # [DEF: 1] If true all processes can print debug logs (otherwise, only the coordinator)
2) FAIL_ON_ERROR   # [DEF: 0] The process will automatically fail on the first error if true
3) FAIL_ON_WARN    # [DEF: 0] The process will automatically fail on the first warning if true
4) DEBUGGING_ON    # [DEF: 0] Enables debug logs (DEBUG)
5) WATERMARK_ON    # [DEF: 1] CSD watermark at the start
6) FLUSH_OUTPUT    # [DEF: 0] If true all debug (DEBUG & ERROR) output will be flushed
7) COLORS          # [DEF: 1] Use terminal colors

- Επίσης μπορεί να γίνει configure ώστε να χρησιμοποιεί hostfile (από default τρέχει χωρίς hostfile), συμπληρώνοντας το όνομά του στο πεδίο
    "HOSTFILE"

- Targets

Παρέχω επίσης 5 targets:

1) clean - Καθαρίζει τα build files
2) all   - Κάνει build το executable
3) test0 - Τρέχει το executable μέσω mpirun για το test 0 με το κατάλληλο configuration np & num_libs
4) test1 - Τρέχει το executable μέσω mpirun για το test 1 με το κατάλληλο configuration np & num_libs
5) test2 - Τρέχει το executable μέσω mpirun για το test 2 με το κατάλληλο configuration np & num_libs


# Εκτέλεση #

- Μπορεί να χρησιμοποιηθεί το Makefile για τα 3 τεστ που δόθηκαν, ή εναλλακτικά manually μέσω της εντολής
    "mpirun -np <NP> ./build/main <Testfile> <NUM_LIBS>".

- Όπως αναφέρεται πιο πάνω, μπορεί να χρησιμοποιηθεί και hostfile είτε μέσω configuration του Makefile, είτε
    με την προσθήκη του --hostfile <Hostfile> flag στο mpirun.
