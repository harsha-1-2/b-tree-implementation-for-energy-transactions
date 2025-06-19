#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define ORDER 6

// Forward declarations
typedef struct BTree BTree;

// Transaction struct
typedef struct Transaction 
{
    int transaction_id;
    int buyer_id;
    int seller_id;
    float energy_kwh;
    float price_per_kwh;
    float total_price;
    time_t timestamp;
} Transaction;

// Regular Buyer Node for Linked List
typedef struct RegularBuyerNode 
{
    int buyer_id;
    int transaction_count;
    struct RegularBuyerNode* next;
} RegularBuyerNode;

// Seller struct
typedef struct Seller 
{
    int seller_id;
    float rate_below_300;
    float rate_above_300;
    RegularBuyerNode* regular_buyers; // Linked list of regular buyers
    float total_revenue;              // Added revenue tracking
    BTree* transaction_subtree;       // B+ tree of transactions
} Seller;

// Buyer struct
typedef struct Buyer 
{
    int buyer_id;
    float total_energy_purchased;     // Sum of all energy bought
    BTree* transaction_subtree;       // B+ tree of transactions
} Buyer;

// Seller-Buyer Pair struct
typedef struct SellerBuyerPair 
{
    int seller_id;
    int buyer_id;
    int number_of_transactions;
} SellerBuyerPair;

// B+ Tree Node
typedef struct Node 
{
    int* keys;                    // Array of keys
    int t;                        // Minimum degree
    struct Node** children;       // Array of child pointers
    int n;                        // Current number of keys
    bool leaf;                    // True if leaf node
    struct Node* next;            // For leaf node chaining
    void** records;               // Array of records (void* for flexibility)
} Node;

// B+ Tree
struct BTree 
{
    Node* root;
    int t;                        // Minimum degree
    char type;                    // 'T' for Transaction, 'S' for Seller, 'B' for Buyer, 'P' for SellerBuyerPair
};

// Create a new node
Node* createNode(int t, bool leaf) 
{
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) 
    {
        printf("Memory allocation failed for Node\n");
        exit(1);
    }
    
    newNode->t = t;
    newNode->leaf = leaf;
    
    newNode->keys = (int*)malloc((2 * t - 1) * sizeof(int));
    newNode->children = (Node**)malloc((2 * t) * sizeof(Node*));
    newNode->n = 0;
    newNode->next = NULL;
    newNode->records = (void**)malloc((2 * t - 1) * sizeof(void*));
    
    return newNode;
}

// Create a new B+ Tree
BTree* createBTree(int t, char type) 
{
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) 
    {
        printf("Memory allocation failed for BTree\n");
        exit(1);
    }
    
    tree->root = createNode(t, true);
    tree->t = t;
    tree->type = type;
    
    return tree;
}

// Split child node for B+ tree
void splitChild(Node* parent, int i, Node* child) 
{
    int t = child->t;
    Node* newChild = createNode(t, child->leaf);
    newChild->n = t-1;
    
    // Copy the keys and records from child to newChild
    for (int j = 0; j < t-1; j++) 
    {
        newChild->keys[j] = child->keys[j + t];
        if (child->leaf) 
        {
            newChild->records[j] = child->records[j + t];
        }
    }
    
    // If not leaf, copy the children pointers
    if (!child->leaf) 
    {
        for (int j = 0; j < t; j++) 
        {
            newChild->children[j] = child->children[j + t];
        }
    }
    
    // Update child's key count
    child->n = t;
    
    // Shift parent's children to accommodate new child
    for (int j = parent->n; j >= i + 1; j--) 
    {
        parent->children[j + 1] = parent->children[j];
    }
    
    // Assign the new child to parent
    parent->children[i + 1] = newChild;
    
    // Shift parent's keys to accommodate new key
    for (int j = parent->n - 1; j >= i; j--) 
    {
        parent->keys[j + 1] = parent->keys[j];
        parent->records[j + 1] = parent->records[j];
    }
    
    // Place the middle key of child into parent
    parent->keys[i] = child->keys[t - 1];
    if (child->leaf) 
    {
        parent->records[i] = child->records[t - 1];
    } 
    else 
    {
        parent->records[i] = NULL; // Internal nodes don't store records in B+ tree
    }
    parent->n++;
    
    // If leaf nodes, update the leaf node chain
    if (child->leaf) 
    {
        newChild->next = child->next;
        child->next = newChild;
    }
}

// Generic insert non-full function
void insertNonFull(Node* node, int key, void* record) 
{
    int i = node->n - 1;
    
    if (node->leaf) 
    {
        // Find position to insert the new key
        while (i >= 0 && key <node->keys[i]) 
        {
            node->keys[i + 1] = node->keys[i];
            node->records[i + 1] = node->records[i];
            i--;
        }
        
        // Insert the new key and record
        node->keys[i + 1] = key;
        node->records[i + 1] = record;
        node->n++;
    }
    else 
    {
        // Find the child which is going to have the new key
        while (i >= 0 && key < node->keys[i]) i--;
        i++;
        
        // See if the found child is full
        if (node->children[i]->n == 2 * node->t - 1) 
        {
            // If the child is full, split it
            splitChild(node, i, node->children[i]);
            
            // After split, decide which child gets the new key
            if (key > node->keys[i]) i++;
        }
        
        // Recursively insert into the appropriate child
        insertNonFull(node->children[i], key, record);
    }
}

// Generic insert function
void insert(BTree* tree, int key, void* record) 
{
    Node* r = tree->root;
    
    // If root is full, create a new root
    if (r->n == 2 * tree->t - 1) 
    {
        Node* s = createNode(tree->t, false);
        s->children[0] = r;
        tree->root = s;
        splitChild(s, 0, r);
        insertNonFull(s, key, record);
    } else {
        insertNonFull(r, key, record);
    }
}

// Search for a record in a B+ Tree by key
void* search(Node* node, int key) 
{
    int i = 0;
    
    // Find the first key greater than or equal to key
    while (i < node->n && key > node->keys[i]) i++;
    
    // If found key equals search key, return its record
    if (i < node->n && node->keys[i] == key) 
    {
        return node->records[i];
    } 
    // If leaf node and key not found, return NULL
    else if (node->leaf) 
    {
        return NULL;
    } 
    // Otherwise, search the appropriate child
    else 
    {
        return search(node->children[i], key);
    }
}

// Create a new transaction
Transaction* createTransaction(int id, int buyer_id, int seller_id, float energy_kwh, float price_per_kwh,time_t timestamp) 
{
    Transaction* tx = (Transaction*)malloc(sizeof(Transaction));
    if (!tx) 
    {
        printf("Memory allocation failed for Transaction\n");
        exit(1);
    }
    
    tx->transaction_id = id;
    tx->buyer_id = buyer_id;
    tx->seller_id = seller_id;
    tx->energy_kwh = energy_kwh;
    tx->price_per_kwh = price_per_kwh;
    tx->total_price = tx->energy_kwh * tx->price_per_kwh;
    tx->timestamp = timestamp;
    
    return tx;
}

// Create a new seller
Seller* createSeller(int seller_id, float rate_below_300, float rate_above_300) 
{
    Seller* seller = (Seller*)malloc(sizeof(Seller));
    if (!seller) 
    {
        printf("Memory allocation failed for Seller\n");
        exit(1);
    }
    
    seller->seller_id = seller_id;
    seller->rate_below_300 = rate_below_300;
    seller->rate_above_300 = rate_above_300;
    seller->regular_buyers = NULL;
    seller->total_revenue = 0.0;
    seller->transaction_subtree = createBTree(2, 'T');
    
    return seller;
}

// Create a new buyer
Buyer* createBuyer(int buyer_id) 
{
    Buyer* buyer = (Buyer*)malloc(sizeof(Buyer));
    if (!buyer) 
    {
        printf("Memory allocation failed for Buyer\n");
        exit(1);
    }
    
    buyer->buyer_id = buyer_id;
    buyer->total_energy_purchased = 0.0;
    buyer->transaction_subtree = createBTree(2, 'T');
    
    return buyer;
}

// Create a new regular buyer node for linked list
RegularBuyerNode* createRegularBuyerNode(int buyer_id) 
{
    RegularBuyerNode* node = (RegularBuyerNode*)malloc(sizeof(RegularBuyerNode));
    if (!node) 
    {
        printf("Memory allocation failed for RegularBuyerNode\n");
        exit(1);
    }
    
    node->buyer_id = buyer_id;
    node->transaction_count = 1;  // Start with 1 transaction
    node->next = NULL;
    
    return node;
}

// Create a new seller-buyer pair
SellerBuyerPair* createSellerBuyerPair(int seller_id, int buyer_id) 
{
    SellerBuyerPair* pair = (SellerBuyerPair*)malloc(sizeof(SellerBuyerPair));
    if (!pair) 
    {
        printf("Memory allocation failed for SellerBuyerPair\n");
        return pair;
    }
    
    pair->seller_id = seller_id;
    pair->buyer_id = buyer_id;
    pair->number_of_transactions = 1; // Start with 1 transaction
    
    return pair;
}

// Insert a transaction
void insertTransaction(BTree* tree, Transaction* tx) 
{
    insert(tree, tx->transaction_id, tx);
}

// Insert a seller
void insertSeller(BTree* tree, Seller* seller) 
{
    insert(tree, seller->seller_id, seller);
}

// Insert a buyer
void insertBuyer(BTree* tree, Buyer* buyer) 
{
    insert(tree, buyer->buyer_id, buyer);
}

// Create a key for SellerBuyerPair (combine seller_id and buyer_id)
int createPairKey(int seller_id, int buyer_id)
{
    // Simple hash: shift seller_id left by 16 bits and OR with buyer_id
    return (seller_id << 16) | (buyer_id & 0xFFFF);
}

// Insert a seller-buyer pair
void insertSellerBuyerPair(BTree* tree, SellerBuyerPair* pair) 
{
    int key = createPairKey(pair->seller_id, pair->buyer_id);
    insert(tree, key, pair);
}

// Search for a transaction
Transaction* searchTransaction(BTree* tree, int transaction_id) 
{
    return (Transaction*)search(tree->root, transaction_id);
}

// Search for a seller
Seller* searchSeller(BTree* tree, int seller_id) 
{
    return (Seller*)search(tree->root, seller_id);
}

// Search for a buyer
Buyer* searchBuyer(BTree* tree, int buyer_id) 
{
    return (Buyer*)search(tree->root, buyer_id);
}

// Search for a seller-buyer pair
SellerBuyerPair* searchSellerBuyerPair(BTree* tree, int seller_id, int buyer_id) 
{
    int key = createPairKey(seller_id, buyer_id);
    return (SellerBuyerPair*)search(tree->root, key);
}

// Add or update a regular buyer in seller's list
void addRegularBuyer(Seller* seller, int buyer_id) 
{
    // Check if buyer already exists in the list
    RegularBuyerNode* current = seller->regular_buyers;
    RegularBuyerNode* prev = NULL;
    
    while (current != NULL) 
    {
        if (current->buyer_id == buyer_id) 
        {
            // Buyer exists, increment transaction count
            current->transaction_count++;
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Buyer not found, create a new node
    RegularBuyerNode* newNode = createRegularBuyerNode(buyer_id);
    
    // Add to the beginning of the list if it's empty or at the end if not
    if (seller->regular_buyers == NULL) 
    {
        seller->regular_buyers = newNode;
    } 
    else 
    {
        prev->next = newNode;
    }
}

// Clean up regular buyers list to only include those with more than 5 transactions
void cleanupRegularBuyers(Seller* seller) 
{
    RegularBuyerNode* current = seller->regular_buyers;
    RegularBuyerNode* prev = NULL;
    RegularBuyerNode* next;
    
    while (current != NULL) 
    {
        next = current->next;
        
        if (current->transaction_count < 5) 
        {
            // Remove this node
            if (prev == NULL) 
            {
                // This is the head node
                seller->regular_buyers = next;
            } 
            else 
            {
                prev->next = next;
            }
            free(current);
        } 
        else 
        {
            prev = current;
        }
        
        current = next;
    }
}

// Process a transaction - update related data structures
void processTransaction(Transaction* tx, BTree* sellerTree, BTree* buyerTree, BTree* pairTree) 
{
    // Find or create seller
    Seller* seller = searchSeller(sellerTree, tx->seller_id);
    if (!seller) 
    {
        seller = createSeller(tx->seller_id,0,0); // Default rates
        insertSeller(sellerTree, seller);
    }

    //Update seller's rates
    if(tx->energy_kwh<300 && seller->rate_below_300==0) seller->rate_below_300=tx->price_per_kwh;
    else if(tx->energy_kwh>=300 && seller->rate_above_300==0) seller->rate_above_300=tx->price_per_kwh;
    
    // Update seller's revenue
    seller->total_revenue += tx->total_price;
    
    // Add transaction to seller's transaction subtree
    insertTransaction(seller->transaction_subtree, tx);
    
    // Find or create buyer
    Buyer* buyer = searchBuyer(buyerTree, tx->buyer_id);
    if (!buyer) 
    {
        buyer = createBuyer(tx->buyer_id);
        insertBuyer(buyerTree, buyer);
    }
    
    // Update buyer's total energy purchased
    buyer->total_energy_purchased += tx->energy_kwh;
    
    // Add transaction to buyer's transaction subtree
    insertTransaction(buyer->transaction_subtree, tx);
    
    // Update regular buyers list
    addRegularBuyer(seller, tx->buyer_id);
    
    // Clean up regular buyers list (only keep those with 5+ transactions)
    cleanupRegularBuyers(seller);
    
    // Update or create seller-buyer pair
    SellerBuyerPair* pair = searchSellerBuyerPair(pairTree, tx->seller_id, tx->buyer_id);
    
    if (pair) 
    {
        // Pair exists, increment transaction count
        pair->number_of_transactions++;
    } 
    else 
    {
        // Create new pair
        pair = createSellerBuyerPair(tx->seller_id, tx->buyer_id);
        insertSellerBuyerPair(pairTree, pair);
    }
}

// Function to traverse and display all transactions in the B+ tree
void displayAllTransactions(BTree* tree) 
{
    if (!tree || !tree->root) 
    {
        printf("Transaction tree is empty.\n");
        return;
    }
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Traverse all leaf nodes using the 'next' pointers
    printf("\n===== TRANSACTION LIST =====\n");
    printf("%-6s | %-8s | %-8s | %-15s | %-15s | %-15s | %-20s\n", 
           "TX ID", "BUYER ID", "SELLER ID", "ENERGY (kWh)", "PRICE/kWh", "TOTAL PRICE", "TIMESTAMP");
    printf("--------------------------------------------------------------------------------------\n");
    
    int count = 0;
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            Transaction* tx = (Transaction*)current->records[i];
            char time_str[30];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&tx->timestamp));
            
            printf("%-6d | %-8d | %-8d | %-15.2f | %-15.2f | %-15.2f | %-20s\n", 
                   tx->transaction_id, tx->buyer_id, tx->seller_id, 
                   tx->energy_kwh, tx->price_per_kwh, tx->total_price, time_str);
            count++;
        }
        current = current->next;
    }
    
    printf("--------------------------------------------------------------------------------------\n");
    printf("Total transactions: %d\n\n", count);
}

// Process and display a seller record
void displaySellerRecord(void* record) 
{
    Seller* seller = (Seller*)record;
    printf("%-8d | %-15.4f | %-15.4f | %-12.2f | ", 
           seller->seller_id, 
           seller->rate_below_300, 
           seller->rate_above_300,
           seller->total_revenue);
    
    // Display regular buyers
    printf("Regular buyers: ");
    int buyerCount = 0;
    RegularBuyerNode* current = seller->regular_buyers;
    while (current != NULL) {
        if (buyerCount > 0) printf(", ");
        printf("%d(%d tx)", current->buyer_id, current->transaction_count);
        buyerCount++;
        current = current->next;
    }
    if (buyerCount == 0) printf("None");
    printf("\n");
}

// Display all sellers
void displayAllSellers(BTree* tree) 
{
    if (!tree || !tree->root) 
    {
        printf("Seller tree is empty.\n");
        return;
    }
    
    printf("\n===== SELLER LIST =====\n");
    printf("%-8s | %-15s | %-15s | %-12s | %-s\n", 
           "SELLER ID", "RATE <300kWh", "RATE >300kWh", "REVENUE", "REGULAR BUYERS");
    printf("--------------------------------------------------------------------------------------\n");
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Traverse all leaf nodes using the 'next' pointers
    int count = 0;
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            displaySellerRecord(current->records[i]);
            count++;
        }
        current = current->next;
    }
    
    printf("--------------------------------------------------------------------------------------\n");
    printf("Total sellers: %d\n\n", count);
}

// Process and display a buyer record
void displayBuyerRecord(void* record) 
{
    Buyer* buyer = (Buyer*)record;
    printf("%-8d | %-20.2f\n", 
           buyer->buyer_id, 
           buyer->total_energy_purchased);
}

// Display all buyers
void displayAllBuyers(BTree* tree) 
{
    if (!tree || !tree->root) 
    {
        printf("Buyer tree is empty.\n");
        return;
    }
    
    printf("\n===== BUYER LIST =====\n");
    printf("%-8s | %-20s\n", "BUYER ID", "TOTAL ENERGY (kWh)");
    printf("---------------------------------\n");
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Traverse all leaf nodes using the 'next' pointers
    int count = 0;
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            displayBuyerRecord(current->records[i]);
            count++;
        }
        current = current->next;
    }
    
    printf("---------------------------------\n");
    printf("Total buyers: %d\n\n", count);
}

/**
 * Display transactions within a specific time range
 */
void displayTransactionsInTimeRange(BTree* tree, time_t start_time, time_t end_time) 
{
    if (!tree || !tree->root) 
    {
        printf("Transaction tree is empty.\n");
        return;
    }
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Format time strings for display
    char start_str[30], end_str[30];
    strftime(start_str, sizeof(start_str), "%Y-%m-%d %H:%M:%S", localtime(&start_time));
    strftime(end_str, sizeof(end_str), "%Y-%m-%d %H:%M:%S", localtime(&end_time));
    
    // Traverse all leaf nodes using the 'next' pointers
    printf("\n===== TRANSACTIONS FROM %s TO %s =====\n", start_str, end_str);
    printf("%-6s | %-8s | %-8s | %-15s | %-15s | %-15s | %-20s\n", 
           "TX ID", "BUYER ID", "SELLER ID", "ENERGY (kWh)", "PRICE/kWh", "TOTAL PRICE", "TIMESTAMP");
    printf("--------------------------------------------------------------------------------------\n");
    
    int count = 0;
    float total_energy = 0.0;
    float total_revenue = 0.0;
    
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            Transaction* tx = current->records[i];
            
            // Check if transaction is within the specified time range
            if (tx->timestamp >= start_time && tx->timestamp <= end_time) 
            {
                char time_str[30];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&tx->timestamp));
                
                printf("%-6d | %-8d | %-8d | %-15.2f | %-15.2f | %-15.2f | %-20s\n", 
                       tx->transaction_id, tx->buyer_id, tx->seller_id, 
                       tx->energy_kwh, tx->price_per_kwh, tx->total_price, time_str);
                
                count++;
                total_energy += tx->energy_kwh;
                total_revenue += tx->total_price;
            }
        }
        current = current->next;
    }
    
    printf("--------------------------------------------------------------------------------------\n");
    printf("Total transactions: %d | Total energy: %.2f kWh | Total revenue: $%.2f\n\n", 
           count, total_energy, total_revenue);
}

//Calculate total revenue for a specific seller
 float calculateSellerRevenue(BTree* tree, int seller_id) 
{
    if (!tree || !tree->root) 
    {
        printf("Transaction tree is empty.\n");
        return 0.0;
    }
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Traverse all leaf nodes and sum up revenue for the specified seller
    float total_revenue = 0.0;
    int transaction_count = 0;
    float total_energy_sold = 0.0;
    
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            Transaction* tx = current->records[i];
            
            // Check if this transaction involves the seller we're looking for
            if (tx->seller_id == seller_id) 
            {
                total_revenue += tx->total_price;
                total_energy_sold += tx->energy_kwh;
                transaction_count++;
            }
        }
        current = current->next;
    }
    
    printf("\n===== REVENUE SUMMARY FOR SELLER ID: %d =====\n", seller_id);
    printf("Total transactions: %d\n", transaction_count);
    printf("Total energy sold: %.2f kWh\n", total_energy_sold);
    printf("Total revenue: $%.2f\n\n", total_revenue);
    
    return total_revenue;
}


 // Find and display transactions in ascending order by energy amount within a range
void displayTransactionsByEnergyRange(BTree* tree, float min_energy, float max_energy)
{
    if (!tree || !tree->root) 
    {
        printf("Transaction tree is empty.\n");
        return;
    }
    
    // Step 1: Traverse tree and collect transactions in the energy range
    Transaction** transactions = NULL;
    int count = 0;
    int capacity = 10;  // Initial capacity
    
    // Allocate initial array
    transactions = (Transaction**)malloc(capacity * sizeof(Transaction*));
    if (!transactions) {
        printf("Memory allocation failed.\n");
        return;
    }
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Collect transactions in range
    while (current != NULL) {
        for (int i = 0; i < current->n; i++) 
        {
            Transaction* tx = (Transaction*)current->records[i];
            
            if (tx->energy_kwh >= min_energy && tx->energy_kwh <= max_energy) 
            {
                // Resize array if needed
                if (count >= capacity) 
                {
                    capacity *= 2;
                    transactions = (Transaction**)realloc(transactions, capacity * sizeof(Transaction*));
                    if (!transactions) 
                    {
                        printf("Memory reallocation failed.\n");
                        return;
                    }
                }
                
                transactions[count++] = tx;
            }
        }
        current = current->next;
    }
    
    // Step 2: Sort transactions by energy (Insertion Sort)
    for (int i = 1; i < count; i++) 
    {
        Transaction* key = transactions[i];
        int j = i - 1;
        
        while (j >= 0 && transactions[j]->energy_kwh > key->energy_kwh) 
        {
            transactions[j + 1] = transactions[j];
            j--;
        }
        
        transactions[j + 1] = key;
    }
    
    // Step 3: Display sorted transactions
    printf("\n===== TRANSACTIONS BY ENERGY RANGE (%.2f - %.2f kWh) =====\n", min_energy, max_energy);
    printf("%-6s | %-8s | %-8s | %-15s | %-15s | %-15s | %-20s\n", 
           "TX ID", "BUYER ID", "SELLER ID", "ENERGY (kWh)", "PRICE/kWh", "TOTAL PRICE", "TIMESTAMP");
    printf("--------------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) 
    {
        Transaction* tx = transactions[i];
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&tx->timestamp));
        
        printf("%-6d | %-8d | %-8d | %-15.2f | %-15.2f | %-15.2f | %-20s\n", 
               tx->transaction_id, tx->buyer_id, tx->seller_id, 
               tx->energy_kwh, tx->price_per_kwh, tx->total_price, time_str);
    }
    
    printf("--------------------------------------------------------------------------------------\n");
    printf("Total transactions in range: %d\n\n", count);
    
    // Free the auxiliary array
    free(transactions);
}


 //Display buyers in ascending order based on energy bought
void displayBuyersByEnergyBought(BTree* buyerTree) 
{
    if (!buyerTree || !buyerTree->root) 
    {
        printf("Buyer tree is empty.\n");
        return;
    }
    
    // Step 1: Traverse tree and collect buyers
    Buyer** buyers = NULL;
    int count = 0;
    int capacity = 10;  // Initial capacity
    
    // Allocate initial array
    buyers = (Buyer**)malloc(capacity * sizeof(Buyer*));
    if (!buyers) 
    {
        printf("Memory allocation failed.\n");
        return;
    }
    
    // Find the leftmost leaf node
    Node* current = buyerTree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Collect buyers
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) {
            Buyer* buyer = (Buyer*)current->records[i];
            
            // Resize array if needed
            if (count >= capacity) 
            {
                capacity *= 2;
                buyers = (Buyer**)realloc(buyers, capacity * sizeof(Buyer*));
                if (!buyers) 
                {
                    printf("Memory reallocation failed.\n");
                    return;
                }
            }
            
            buyers[count++] = buyer;
        }
        current = current->next;
    }
    
    // Step 2: Sort buyers by energy purchased (Insertion Sort)
    for (int i = 1; i < count; i++) 
    {
        Buyer* key = buyers[i];
        int j = i - 1;
        
        while (j >= 0 && buyers[j]->total_energy_purchased > key->total_energy_purchased) 
        {
            buyers[j + 1] = buyers[j];
            j--;
        }
        
        buyers[j + 1] = key;
    }
    
    // Step 3: Display sorted buyers
    printf("\n===== BUYERS BY ENERGY PURCHASED (ASCENDING) =====\n");
    printf("%-8s | %-20s\n", "BUYER ID", "TOTAL ENERGY (kWh)");
    printf("---------------------------------\n");
    
    for (int i = 0; i < count; i++) 
    {
        printf("%-8d | %-20.2f\n", 
               buyers[i]->buyer_id, 
               buyers[i]->total_energy_purchased);
    }
    
    printf("---------------------------------\n");
    printf("Total buyers: %d\n\n", count);
    
    // Free the auxiliary array
    free(buyers);
}


// Display seller-buyer pairs in ascending order based on number of transactions
void displayPairsByTransactionCount(BTree* pairTree) 
{
    if (!pairTree || !pairTree->root) 
    {
        printf("Seller-Buyer pair tree is empty.\n");
        return;
    }
    
    // Step 1: Traverse tree and collect pairs
    SellerBuyerPair** pairs = NULL;
    int count = 0;
    int capacity = 10;  // Initial capacity
    
    // Allocate initial array
    pairs = (SellerBuyerPair**)malloc(capacity * sizeof(SellerBuyerPair*));
    if (!pairs) 
    {
        printf("Memory allocation failed.\n");
        return;
    }
    
    // Find the leftmost leaf node
    Node* current = pairTree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Collect pairs
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) {
            SellerBuyerPair* pair = (SellerBuyerPair*)current->records[i];
            
            // Resize array if needed
            if (count >= capacity) 
            {
                capacity *= 2;
                pairs = (SellerBuyerPair**)realloc(pairs, capacity * sizeof(SellerBuyerPair*));
                if (!pairs) 
                {
                    printf("Memory reallocation failed.\n");
                    return;
                }
            }
            
            pairs[count++] = pair;
        }
        current = current->next;
    }
    
    // Step 2: Sort pairs by transaction count (Insertion Sort)
    for (int i = 1; i < count; i++) 
    {
        SellerBuyerPair* key = pairs[i];
        int j = i - 1;
        
        while (j >= 0 && pairs[j]->number_of_transactions > key->number_of_transactions) 
        {
            pairs[j + 1] = pairs[j];
            j--;
        }
        
        pairs[j + 1] = key;
    }
    
    // Step 3: Display sorted pairs
    printf("\n===== SELLER-BUYER PAIRS BY TRANSACTION COUNT (ASCENDING) =====\n");
    printf("%-8s | %-8s | %-20s\n", "SELLER ID", "BUYER ID", "TRANSACTION COUNT");
    printf("------------------------------------------\n");
    
    for (int i = 0; i < count; i++) 
    {
        printf("%-8d | %-8d | %-20d\n", 
               pairs[i]->seller_id, 
               pairs[i]->buyer_id, 
               pairs[i]->number_of_transactions);
    }
    
    printf("------------------------------------------\n");
    printf("Total pairs: %d\n\n", count);
    
    // Free the auxiliary array
    free(pairs);
}

int my_strptime(const char *date_str, const char *format, struct tm *tm) 
{
    // This function handles the "%Y-%m-%d" format.
    int year, month, day;
    if (strcmp(format, "%Y-%m-%d") == 0) 
    {
        if (sscanf(date_str, "%4d-%2d-%2d", &year, &month, &day) == 3) 
        {
            tm->tm_year = year - 1900;  // years since 1900
            tm->tm_mon = month - 1;     // months since January [0-11]
            tm->tm_mday = day;
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
            tm->tm_isdst = -1;
            return 1;
        }
    }
    return 0;
}

//Function to import data from transactions.txt
void importTransactions(BTree* transactionTree, BTree* sellerTree, BTree* buyerTree, BTree* pairTree) 
{
    FILE* file = fopen("transactions.txt", "r");
    if (!file) 
    {
        printf("Error opening file transactions.txt for reading\n");
        return;
    }
    
    int count = 0;
    char line[256];
    
    // Skip header line if present
    if (fgets(line, sizeof(line), file) != NULL) 
    {
        // Check if this is a header line (contains non-numeric characters)
        if (line[0] < '0' || line[0] > '9') 
        {
            // It's a header, continue to next line
        } 
        else 
        {
            // It's data, rewind to start of file
            rewind(file);
        }
    }
    
    // Read and process each line
    while (fgets(line, sizeof(line), file) != NULL) 
    {
        int transaction_id, buyer_id, seller_id;
        float energy_kwh, price_per_kwh;
        time_t timestamp;
        
        // Parse the line
        if (sscanf(line, "%d,%d,%d,%f,%f,%ld", &transaction_id, &buyer_id, &seller_id, &energy_kwh, &price_per_kwh, &timestamp) == 6)
        {
            
            // Create and process the transaction
            Transaction* tx = createTransaction(transaction_id, buyer_id, seller_id, energy_kwh, price_per_kwh, timestamp);
            
            // Add to transaction tree
            insertTransaction(transactionTree, tx);
            
            // Update related data structures
            processTransaction(tx, sellerTree, buyerTree, pairTree);
            
            count++;
        } 
        else 
        {
            printf("Warning: Skipping invalid line: %s", line);
        }
    }
    
    fclose(file);
    printf("Successfully imported %d transactions from transactions.txt\n", count );
}


void exportTransactions( BTree* tree) 
{
    FILE* file = fopen("transactions.txt", "w");
    if (!file) 
    {
        printf("Error opening file transactions.txt for writing\n");
        return;
    }
    
    // Write header
    fprintf(file, "transaction_id,buyer_id,seller_id,energy,price,timestamp\n");
    
    if (!tree || !tree->root) 
    {
        printf("Transaction tree is empty, writing empty file.\n");
        fclose(file);
        return;
    }
    
    int count = 0;
    
    // Find the leftmost leaf node
    Node* current = tree->root;
    while (!current->leaf) 
    {
        current = current->children[0];
    }
    
    // Traverse all leaf nodes using the 'next' pointers
    while (current != NULL) 
    {
        for (int i = 0; i < current->n; i++) 
        {
            Transaction* tx = (Transaction*)current->records[i];
            
            // Write transaction data to file
            fprintf(file, "%d,%d,%d,%.2f,%.2f,%ld\n", 
                   tx->transaction_id, tx->buyer_id, tx->seller_id, 
                   tx->energy_kwh, tx->price_per_kwh, tx->timestamp);
            
            count++;
        }
        current = current->next;
    }
    
    fclose(file);
    printf("Successfully exported %d transactions to transactions.txt\n", count);
}


// Main function to demonstrate usage
int main() 
{
    // Create B+ trees for different entity types
    BTree* transactionTree = createBTree(ORDER/2, 'T');
    BTree* sellerTree = createBTree(ORDER/2, 'S');
    BTree* buyerTree = createBTree(ORDER/2, 'B');
    BTree* pairTree = createBTree(ORDER/2, 'P');
    
    importTransactions(transactionTree,sellerTree,buyerTree,pairTree);
    
    int choice = 0,flag=1;
    
    
    while (flag)
    {
        // Display menu
        printf("\n===== ENERGY TRADING SYSTEM =====\n");
        printf("Operations Menu:\n");
        printf("1. Add New Transactions\n");
        printf("2. Display All Transactions\n");
        printf("3. Create a set of Transactions for Every Seller\n");
        printf("4. Create a set of Transactions for Every Buyer\n");
        printf("5. Find all transactions in a Given Time Period\n");
        printf("6. Calculate Total Revenue by Seller\n");
        printf("7. Find and Display transactions with Energy Amounts in range\n");
        printf("8. Sort the set of Buyers Based on Energy Bought\n");
        printf("9. Sort Seller/Buyer Pairs by Number of Transactions\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) 
        {
            // Clear input buffer on failed read
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        // Process choice
        switch (choice) 
        {
            case 0:
                printf("Exiting system. Goodbye!\n");
                flag=0;
                break;
                
            case 1: 
            { // Add New Transactions
                int txn_id,buyer_id, seller_id;
                float energy_kwh, price_per_kwh;
                
                printf("\n----- Add New Transaction -----\n");
                printf("Enter Transaction ID: ");
                scanf("%d", &txn_id);
                printf("Enter Buyer ID: ");
                scanf("%d", &buyer_id);
                printf("Enter Seller ID: ");
                scanf("%d", &seller_id);
                printf("Enter Energy (kWh): ");
                scanf("%f", &energy_kwh);
                Seller* seller = searchSeller(sellerTree, seller_id);
                if (seller!=NULL) 
                {
                    if(energy_kwh<300 && seller->rate_below_300!=0) 
                    {
                        price_per_kwh=seller->rate_below_300;
                        printf("Price=%f (Auto Renew)\n", price_per_kwh);
                    }
                    else if(energy_kwh>=300 && seller->rate_above_300!=0)
                    {
                        price_per_kwh=seller->rate_above_300;
                        printf("Price=%f (Auto Renew)\n", price_per_kwh);
                    }
                    else 
                    {
                        printf("Enter Price per kWh: ");
                        scanf("%f", &price_per_kwh);
                    }
                    
                } 
                else 
                {
                    printf("Enter Price per kWh: ");
                    scanf("%f", &price_per_kwh);
                }
                
                Transaction* tx = createTransaction(txn_id, buyer_id, seller_id, energy_kwh, price_per_kwh,time(NULL));
                processTransaction(tx, sellerTree, buyerTree, pairTree);
                insertTransaction(transactionTree, tx);
                
                printf("Transaction added successfully with ID: %d\n", tx->transaction_id);
                break;
            }
                
            case 2: // Display All Transactions
                displayAllTransactions(transactionTree);
                break;
                
            case 3: 
            { 
                displayAllSellers(sellerTree);
                break;
            }
                
            case 4: 
            { 
                displayAllBuyers(buyerTree);
                break;
            }
                
            case 5: 
            { // Display transactions in a given time period
                printf("\n----- Transactions in Time Period -----\n");
            
                char start_date[11], end_date[11];
                struct tm start_tm = {0}, end_tm = {0};
            
                printf("Enter start date (YYYY-MM-DD): ");
                scanf("%10s", start_date);
            
                printf("Enter end date (YYYY-MM-DD): ");
                scanf("%10s", end_date);
            
                if (!my_strptime(start_date, "%Y-%m-%d", &start_tm)) 
                {
                    printf("Invalid start date format.\n");
                    break;
                }
            
                if (!my_strptime(end_date, "%Y-%m-%d", &end_tm)) 
                {
                    printf("Invalid end date format.\n");
                    break;
                }
            
                // Set time to cover the full range for the dates
                start_tm.tm_hour = 0; start_tm.tm_min = 0; start_tm.tm_sec = 0;
                end_tm.tm_hour = 23;  end_tm.tm_min = 59;  end_tm.tm_sec = 59;
            
                time_t start_time = mktime(&start_tm);
                time_t end_time = mktime(&end_tm);
            
                if (start_time == -1 || end_time == -1) 
                {
                    printf("Error converting date to time.\n");
                    break;
                }
            
                displayTransactionsInTimeRange(transactionTree, start_time, end_time);
                break;
            }
            
                
            case 6: 
            { // Calculate Total Revenue by Seller
                printf("\n----- Revenue by Seller -----\n");
                printf("Enter Seller ID (0 to see all sellers first): ");
                int seller_id;
                scanf("%d", &seller_id);
                
                if (seller_id == 0) 
                {
                    displayAllSellers(sellerTree);
                    printf("Enter Seller ID: ");
                    scanf("%d", &seller_id);
                }
                
                calculateSellerRevenue(transactionTree, seller_id);
                break;
            }
                
            case 7: 
            { // Find and Display transactions with Energy Amounts in range
                printf("\n----- Transactions by Energy Range -----\n");
                printf("Enter minimum energy amount (kWh): ");
                float min_energy, max_energy;
                scanf("%f", &min_energy);
                printf("Enter maximum energy amount (kWh): ");
                scanf("%f", &max_energy);
                
                displayTransactionsByEnergyRange(transactionTree, min_energy, max_energy);
                break;
            }
                
            case 8: // Sort Buyers Based on Energy Bought
                displayBuyersByEnergyBought(buyerTree);
                break;
                
            case 9: // Sort Seller/Buyer Pairs by Number of Transactions
                displayPairsByTransactionCount(pairTree);
                break;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
    exportTransactions(transactionTree);
    return 0;
}
