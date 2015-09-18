#include"header.h"
struct node* R=NULL;
struct node* S=NULL;
unsigned long numOfNodes;

struct timespec diff(timespec start, timespec end) //get difference in time in nano seconds
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) 
	{
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else 
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

static inline struct node* getAddress(struct node* p)
{
	return (struct node*)((uintptr_t) p & ~((uintptr_t) 3));
}

static inline bool isNull(struct node* p)
{
	return ((uintptr_t) p & 2) != 0;
}

static inline bool isLocked(struct node* p)
{
	return ((uintptr_t) p & 1) != 0;
}

static inline struct node* setLock(struct node* p)
{
	return (struct node*) ((uintptr_t) p | 1);
}

static inline struct node* unsetLock(struct node* p)
{
	return (struct node*) ((uintptr_t) p & ~((uintptr_t) 1));
}

static inline struct node* setNull(struct node* p)
{
	return (struct node*) ((uintptr_t) p | 2);
}

static inline bool CAS(struct node* parent, int which, struct node* oldChild, struct node* newChild)
{
	if(parent->child[which] == oldChild)
	{
		return(parent->child[which].compare_and_swap(newChild,oldChild) == oldChild);
	}
	return false;
}

static inline struct node* newLeafNode(unsigned long key)
{
  struct node* node = (struct node*) malloc(sizeof(struct node));
  node->key = key;
  node->child[LEFT] = setNull(NULL); 
  node->child[RIGHT] = setNull(NULL);
  return(node);
}

void createHeadNodes()
{
  R = newLeafNode(0);
  R->child[RIGHT] = newLeafNode(ULONG_MAX);
  S = R->child[RIGHT];
}

static inline bool lockEdge(struct node* parent, struct node* oldChild, int which, bool n)
{
	if(isLocked(parent->child[which]))
	{
		return false;
	}
	struct node* newChild;
  newChild = setLock(oldChild);
	if(n)
	{
		if(CAS(parent, which, setNull(oldChild), setNull(newChild)))
		{
			return true;
		}
	}
	else
	{
		if(CAS(parent, which, oldChild, newChild))
		{
			return true;
		}
	}
  return false;
}

static inline void unlockEdge(struct node* parent, int which)
{
  parent->child[which] = unsetLock(parent->child[which]);
}

__inline void seek(struct tArgs* t,unsigned long key)
{
	struct timespec s,e;
	clock_gettime(CLOCK_REALTIME,&s);
	struct node* prev[2];
	struct node* curr[2];
	struct node* lastRNode[2];
	struct node* address[2];
	unsigned long lastRKey[2];
	unsigned long cKey;
	struct node* temp;
	int which;
	int pSeek;
	int cSeek;
	int index;
	
	pSeek=0; cSeek=1;
	prev[pSeek] = NULL;	curr[pSeek] = NULL;
	lastRNode[pSeek] = NULL;	lastRKey[pSeek] = 0;
	address[pSeek] = NULL;
	
	BEGIN:
	{	
		//initialize all variables use in traversal
		prev[cSeek] = R; curr[cSeek] = S;
		lastRNode[cSeek] = R; lastRKey[cSeek] = 0;
		address[cSeek] = NULL;
		
		while(true)
		{
			//read the key stored in the current node
			cKey = curr[cSeek]->key;
			if(key<cKey)
			{
				which = LEFT;
			}
			else if(key > cKey)
			{
				which = RIGHT;
			}
			else
			{
				//key found; stop the traversal
				index = cSeek;
				goto END;
			}
			temp = curr[cSeek]->child[which];		
			address[cSeek] = getAddress(temp);
			if(isNull(temp))
			{
				//null flag set; reached a leaf node
				if(lastRNode[cSeek]->key != lastRKey[cSeek])
				{
					t->readRetries++;
					goto BEGIN;
				}
				if(!isLocked(lastRNode[cSeek]->child[RIGHT]))
				{
					//key stored in the node at which the last right edge was traversed has not changed
					index = cSeek;
					goto END;
				}
				else if(lastRNode[pSeek] == lastRNode[cSeek] && lastRKey[pSeek] == lastRKey[cSeek])
				{
					index = pSeek;
					goto END;
				}
				else
				{
					pSeek = 1-pSeek;
					cSeek = 1-cSeek;
					t->readRetries++;
					goto BEGIN;
				}
			}
			if(which == RIGHT)
			{
				//the next edge that will be traversed is the right edge; keep track of the current node and its key
				lastRNode[cSeek] = curr[cSeek];
				lastRKey[cSeek] = cKey;
			}
			//traverse the next edge
			t->seekLength++;
			prev[cSeek] = curr[cSeek];	curr[cSeek] = address[cSeek];
			//determine if the most recent edge traversed is marked
		}
	}
	END:
	{
		//initialize the seek record and return
		t->mySeekRecord.parent = prev[index];
		t->mySeekRecord.node = curr[index];
		t->mySeekRecord.injectionPoint = address[index];
		clock_gettime(CLOCK_REALTIME,&e);
		t->seekTime.tv_sec += diff(s,e).tv_sec;t->seekTime.tv_nsec += diff(s,e).tv_nsec;
		return;			
	}
}

static inline bool findSmallest(struct tArgs* t, struct node* node, struct node* rChild)
{
	//find the node with the smallest key in the subtree rooted at the right child
	struct node* prev;
	struct node* curr;
	struct node* lChild;
	struct node* temp;
	bool n;
	
	prev=node; curr=rChild;
	while(true)
	{
		temp = curr->child[LEFT];
		n = isNull(temp); lChild = getAddress(temp);
		if(n)
		{
			break;
		}
		//traverse the next edge
		prev = curr; curr = lChild;
	}
	//initialize seek record and return
	t->mySeekRecord.node = curr;
	t->mySeekRecord.parent = prev;
	if(prev == node)
	{
		return true;
	}
	else
	{
		return false;
	}
}

unsigned long search(struct tArgs* t, unsigned long key)
{
  struct node* node;
	seek(t,key);
	node = t->mySeekRecord.node;
  if(key == node->key)
	{
		t->successfulReads++;
		return true;
	}
	else
	{
		t->unsuccessfulReads++;
		return false;
	}
}

bool insert(struct tArgs* t, unsigned long key)
{
	struct node* node;
	struct node* newNode;
	struct node* address;
	unsigned long nKey;
	int which;
	bool result;
	while(true)
	{
		seek(t,key);
		node = t->mySeekRecord.node;
		nKey = node->key;
		if(nKey == key)
		{
			t->unsuccessfulInserts++;
			return(false);
		}
		//create a new node and initialize its fields
		if(!t->isNewNodeAvailable)
		{
			t->newNode = newLeafNode(key);
			t->isNewNodeAvailable = true;
		}
		newNode = t->newNode;
		newNode->key = key;
		which = key<nKey ? LEFT:RIGHT;
		address = t->mySeekRecord.injectionPoint;
		result = CAS(node,which,setNull(address),newNode);
		if(result)
		{
			t->isNewNodeAvailable = false;
			t->successfulInserts++;
			return(true);
		}
		t->insertRetries++;
		pthread_yield();
	}
}

bool remove(struct tArgs* t, unsigned long key)
{
	struct node* node;
	struct node* parent;
	struct node* lChild;
	struct node* rChild;
	struct node* temp;
	unsigned long nKey;
	unsigned long pKey;
	int pWhich;
	bool ln;
	bool rn;
	bool lLock;
	bool rLock;
	
  while(true)
	{
		seek(t,key);
		node = t->mySeekRecord.node;
		nKey = node->key;
		if(key != nKey)
		{
				t->unsuccessfulDeletes++;
				return false;
		}
		parent = t->mySeekRecord.parent;
		pKey = parent->key;
		temp = node->child[LEFT];
		lChild = getAddress(temp); ln = isNull(temp); lLock = isLocked(temp);
		temp = node->child[RIGHT];
		rChild = getAddress(temp); rn = isNull(temp); rLock = isLocked(temp);
		pWhich = nKey < pKey ? LEFT: RIGHT;
		if(ln || rn) //simple delete
		{
			if(lockEdge(parent, node, pWhich, false))
			{
				if(lockEdge(node, lChild, LEFT, ln))
				{
					if(lockEdge(node, rChild, RIGHT, rn))
					{
						if(nKey != node->key)
						{
							unlockEdge(parent, pWhich);
							unlockEdge(node, LEFT);
							unlockEdge(node, RIGHT);
							t->deleteRetries++;
							pthread_yield();
							continue;
						}
						if(ln && rn) //00 case
						{
							parent->child[pWhich] = setNull(node);
						}
						else if(ln) //01 case
						{
							parent->child[pWhich] = rChild;
						}
						else //10 case
						{
							parent->child[pWhich] = lChild;
						}
						t->successfulDeletes++;
						t->simpleDeleteCount++;
						return true;
					}
					else
					{
						unlockEdge(parent, pWhich);
						unlockEdge(node, LEFT);
					}
				}
				else
				{
					unlockEdge(parent, pWhich);
				}
			}
		}
		else //complex delete
		{
			struct node* succNode;
			struct node* succParent;
			struct node* succNodeLChild;
			struct node* succNodeRChild;
			struct node* temp;
			bool isSplCase;
			bool srn;
			
			isSplCase = findSmallest(t,node,rChild);
			succNode = t->mySeekRecord.node;
			succParent = t->mySeekRecord.parent;
			succNodeLChild = getAddress(succNode->child[LEFT]);
			temp = succNode->child[RIGHT];
			srn = isNull(temp); succNodeRChild = getAddress(temp);
			if(!isSplCase)
			{
				if(!lockEdge(node, rChild, RIGHT, false))
				{
					t->deleteRetries++;
					pthread_yield();
					continue;
				}
			}
			if(lockEdge(succParent, succNode, isSplCase, false))
			{
				if(lockEdge(succNode, succNodeLChild, LEFT, true))
				{
					if(lockEdge(succNode, succNodeRChild, RIGHT, srn))
					{
						if(nKey != node->key)
						{
							if(!isSplCase)
							{
								unlockEdge(node,RIGHT);
							}
							unlockEdge(succParent,isSplCase);
							unlockEdge(succNode, LEFT);
							unlockEdge(succNode, RIGHT);
							t->deleteRetries++;
							pthread_yield();
							continue;
						}
						node->key = succNode->key;
						if(srn)
						{
							succParent->child[isSplCase] = setNull(succNode);
						}
						else
						{
							succParent->child[isSplCase] = succNodeRChild;
						}
						if(!isSplCase)
						{
							unlockEdge(node, RIGHT);
						}
						t->successfulDeletes++;
						t->complexDeleteCount++;
						return true;
					}
					else
					{
						unlockEdge(succParent,isSplCase);
						unlockEdge(succNode, LEFT);
					}
				}
				else
				{
					unlockEdge(succParent,isSplCase);
				}
			}
			if(!isSplCase)
			{
				unlockEdge(node,RIGHT);
			}
		}
		t->deleteRetries++;
		pthread_yield();
	}
}

void nodeCount(struct node* node)
{
  if(isNull(node))
  {
    return;
  }
  numOfNodes++;
	nodeCount(node->child[LEFT]);
  nodeCount(node->child[RIGHT]);
}

unsigned long size()
{
  numOfNodes=0;
  nodeCount(S->child[LEFT]);
  return numOfNodes;
}

void printKeysInOrder(struct node* node)
{
  if(isNull(node))
  {
    return;
  }
  printKeysInOrder(getAddress(node)->child[LEFT]);
  printf("%lu\t",node->key);
  printKeysInOrder(getAddress(node)->child[RIGHT]);

}

void printKeys()
{
  printKeysInOrder(R);
  printf("\n");
}

unsigned long getHeight(struct node* node)
{
	if(isNull(node))
	{
		return 0;
	}
	int leftH  = getHeight(getAddress(node)->child[LEFT]);
	int rightH = getHeight(getAddress(node)->child[RIGHT]);
	return leftH > rightH ? 1+leftH: 1+rightH;
}

unsigned long height()
{
	return getHeight(S->child[LEFT]);
}

bool isValidBST(struct node* node, unsigned long min, unsigned long max)
{
  if(isNull(node))
  {
    return true;
  }
  if(node->key > min && node->key < max && isValidBST(node->child[LEFT],min,node->key) && isValidBST(node->child[RIGHT],node->key,max))
  {
    return true;
  }
  return false;
}

bool isValidTree()
{
  return(isValidBST(S->child[LEFT],0,ULONG_MAX));
}