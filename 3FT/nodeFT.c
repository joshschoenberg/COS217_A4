/* A node in an FT */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "nodeFT.h"
#include "checkerFT.h"
#include <stdio.h>

struct node {

   /* this contains the contents of a file node or NULL for a dir */
   void * pvFileContents;
   /* this contains the length of pvFileContents or 0 for dir */
   size_t ulContentsLength;
   /* this is true if the node is a file or false if a directory */
   boolean bisFile;
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   /* must be NULL if a file */
   DynArray_T oDChildren;
};

/*
   Returns whether the node is a file (TRUE) or a directory (FALSE)
*/
boolean Node_isFile(Node_T oNNode)
{
   return oNNode->bisFile;
}

/*
   Returns the size of the contents of file oNNode (0 if not a file)
*/
size_t Node_getFileSize(Node_T oNNode)
{
   return oNNode->ulContentsLength;
}

/*
   Returns File contents for a file oNNode, or null for a directory
*/
void * Node_getContents(Node_T oNNode)
{
   if (oNNode->bisFile)
   {
      return oNNode->pvFileContents;
   }
   return NULL;
}

/*
   If oNNode is a file, this function replaces it's contents with
   pvNewContents and ulNewLength, returning the old contents.
   If oNNode is a directory, this function prints an error to STDERR
   and returns NULL
*/
void * Node_replaceContents(Node_T oNNode, void * pvNewContents,
                              size_t ulNewLength)
{
   void * ret = oNNode->pvFileContents;

   assert(oNNode != NULL);

   if (!(oNNode->bisFile))
   {
      fprintf(stderr,
       "Node with path: %s contents cannot replaced. It's not a file",
       Path_getPathname(Node_getPath(oNNode)));
      return NULL;
   }   
   
   oNNode->pvFileContents = pvNewContents;
   oNNode->ulContentsLength = ulNewLength;

   return ret;
}

/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);

   if (oNParent->bisFile)
   {
      return NOT_A_DIRECTORY;
   }
   

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}


/*
  Creates a new dir with path oPPath and parent oNParent.  Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
  * NOT_A_DIRECTORY if oNParent is a file
*/
int Node_dir_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult) {
   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex = 0;
   int iStatus;

   assert(oPPath != NULL);
   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be a directory */
      if(oNParent->bisFile) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NOT_A_DIRECTORY;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
   }
   psNew->oNParent = oNParent;

   /* initialize the new node */
   psNew->oDChildren = DynArray_new(0);
   if(psNew->oDChildren == NULL) {
      Path_free(psNew->oPPath);
      free(psNew);
      *poNResult = NULL;
      return MEMORY_ERROR;
   }
   psNew->pvFileContents = NULL;
   psNew->ulContentsLength = 0;
   psNew->bisFile = FALSE;

   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }

   *poNResult = psNew;

   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
   assert(CheckerFT_Node_isValid(*poNResult));

   return SUCCESS;
}

/*
  Creates a new file with path oPPath parent oNParent, containing
  pvContents and content length of ulContentSize. Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
  * NOT_A_DIRECTORY if oNParent is a file
*/
int Node_file_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult,
                     void * pvContents, size_t ulContentsSize) {

   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex = 0;
   int iStatus;

   assert(oPPath != NULL);
   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* File cannot be the root */
   if (Path_getDepth(psNew->oPPath) == 1)
   {
      return CONFLICTING_PATH;
   }

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be a directory */
      if(oNParent->bisFile) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NOT_A_DIRECTORY;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new file cannot be root */
      Path_free(psNew->oPPath);
      free(psNew);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   psNew->oNParent = oNParent;

   /* initialize the new node */
   /* File cannot have children */
   psNew->oDChildren = NULL;
   psNew->pvFileContents = pvContents;
   psNew->ulContentsLength = ulContentsSize;
   psNew->bisFile = TRUE;


   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }

   *poNResult = psNew;

   assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
   assert(CheckerFT_Node_isValid(*poNResult));

   return SUCCESS;
}

size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);
   assert(CheckerFT_Node_isValid(oNNode));

   /* remove from parent's list */
   if(oNNode->oNParent != NULL) {
      if(DynArray_bsearch(
            oNNode->oNParent->oDChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
         (void) DynArray_removeAt(oNNode->oNParent->oDChildren,
                                  ulIndex);
   }

   /* recursively remove children */
   if (!(oNNode->bisFile))
   {  
      while(DynArray_getLength(oNNode->oDChildren) != 0) {
         ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
      }
      DynArray_free(oNNode->oDChildren);
   }

   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode);
   ulCount++;
   return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* assert(!(oNParent->bisFile)); */

   if (oNParent->bisFile)
   {
      return FALSE;
   }
   

   /* *pulChildID is the index into oNParent->oDChildren */
   return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   if (oNParent->bisFile)
   {
      return 0;
   }

   return DynArray_getLength(oNParent->oDChildren);
}

int Node_getChild(Node_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   if(oNParent->bisFile)
   {
      poNResult = NULL;
      return NOT_A_DIRECTORY;
   }

   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= Node_getNumChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
      *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
      return SUCCESS;
   }
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}
