/*
  A File Tree is a representation of a hierarchy of directories and files,
  represented as an AO with __ state variables:
*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "nodeFT.h"
#include "checkerFT.h"
#include "ft.h"


/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;



/* --------------------------------------------------------------------

  The FT_traversePath and FT_findNode functions modularize the common
  functionality of going as far as possible down an FT towards a path
  and returning either the node of however far was reached or the
  node if the full path was reached, respectively.
*/

/*
  Traverses the FT starting at the root as far as possible towards
  absolute path oPPath. If able to traverse, returns an int SUCCESS
  status and sets *poNFurthest to the furthest node reached (which may
  be only a prefix of oPPath, or even NULL if the root is NULL).
  Otherwise, sets *poNFurthest to NULL and returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest) {
   int iStatus;
   Path_T oPPrefix = NULL;
   Node_T oNCurr;
   Node_T oNChild = NULL;
   size_t ulDepth;
   size_t i;
   size_t ulChildID;

   assert(oPPath != NULL);
   assert(poNFurthest != NULL);

   /* root is NULL -> won't find anything */
   if(oNRoot == NULL) {
      *poNFurthest = NULL;
      return SUCCESS;
   }

   iStatus = Path_prefix(oPPath, 1, &oPPrefix);
   if(iStatus != SUCCESS) {
      *poNFurthest = NULL;
      return iStatus;
   }

   if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
      Path_free(oPPrefix);
      *poNFurthest = NULL;
      return CONFLICTING_PATH;
   }
   Path_free(oPPrefix);
   oPPrefix = NULL;

   oNCurr = oNRoot;
   ulDepth = Path_getDepth(oPPath);
   for(i = 2; i <= ulDepth; i++) {
      iStatus = Path_prefix(oPPath, i, &oPPrefix);
      if(iStatus != SUCCESS) {
         *poNFurthest = NULL;
         return iStatus;
      }
      if(Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
         /* go to that child and continue with next prefix */
         Path_free(oPPrefix);
         oPPrefix = NULL;
         iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
         if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
         }
         oNCurr = oNChild;
      }
      else {
         /* oNCurr doesn't have child with path oPPrefix:
            this is as far as we can go */
         break;
      }
   }

   Path_free(oPPrefix);
   *poNFurthest = oNCurr;
   return SUCCESS;
}

/*
  Traverses the FT to find a node with absolute path pcPath. Returns a
  int SUCCESS status and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request
 */
static int FT_findNode(const char *pcPath, Node_T *poNResult) {
   Path_T oPPath = NULL;
   Node_T oNFound = NULL;
   int iStatus;

   assert(pcPath != NULL);
   assert(poNResult != NULL);

   if(!bIsInitialized) {
      *poNResult = NULL;
      return INITIALIZATION_ERROR;
   }

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS) {
      *poNResult = NULL;
      return iStatus;
   }

   iStatus = FT_traversePath(oPPath, &oNFound);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      *poNResult = NULL;
      return iStatus;
   }

   if(oNFound == NULL) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   if(Path_comparePath(Node_getPath(oNFound), oPPath) != 0) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   Path_free(oPPath);
   *poNResult = oNFound;
   return SUCCESS;
}
/*--------------------------------------------------------------------*/


int FT_insertDir(const char *pcPath) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   /* The ancestor found is a file */
   if (oNCurr != NULL && Node_isFile(oNCurr)) {
      Path_free(oPPath);
      return NOT_A_DIRECTORY;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_dir_new(oPPrefix, oNCurr, &oNNewNode);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   Path_free(oPPath);
   /* update FT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

boolean FT_containsDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   iStatus = FT_findNode(pcPath, &oNFound);

   if (oNFound != NULL)  {
      if (Node_isFile(oNFound))
         return FALSE;
   }
   return (boolean) (iStatus == SUCCESS);
}


int FT_rmDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   iStatus = FT_findNode(pcPath, &oNFound);

   if(iStatus != SUCCESS)
       return iStatus;

   if (Node_isFile(oNFound))
      return NOT_A_DIRECTORY;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength) {
 int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));   

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus = FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL || oNRoot == NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   
   /* The ancestor found is a file */
   if (Node_isFile(oNCurr)) {
      Path_free(oPPath);
      return NOT_A_DIRECTORY;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time,
      until reaching where the file should go */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      /* insert the new node for this level */
      /* if this is the file, create a new file node. Otherwise,
         create a new directory node */
      if (ulIndex == ulDepth) {
         iStatus = Node_file_new(oPPrefix, oNCurr, &oNNewNode, 
                                                pvContents, ulLength);
      }
      else {
         iStatus = Node_dir_new(oPPrefix, oNCurr, &oNNewNode);
      }
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }
   
   Path_free(oPPath);
   /* update FT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

boolean FT_containsFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   iStatus = FT_findNode(pcPath, &oNFound);

   if (oNFound != NULL)  {
      if (!Node_isFile(oNFound))
         return FALSE;
   }
   return (boolean) (iStatus == SUCCESS);
}

int FT_rmFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   iStatus = FT_findNode(pcPath, &oNFound);

   if(iStatus != SUCCESS)
       return iStatus;

   if (!Node_isFile(oNFound))
      return NOT_A_FILE;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

void *FT_getFileContents(const char *pcPath) {
   int iStatus;
   void *pvContents;
   Node_T oNNode = NULL;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   iStatus = FT_findNode(pcPath, &oNNode);
   
   if (iStatus != SUCCESS) 
      return NULL;
   
   if (!Node_isFile(oNNode))
      return NULL;

   pvContents = Node_getContents(oNNode);

   /* NEEDED? assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount)) */

   return pvContents;
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {
   
   int iStatus;
   void *pvOldContents;
   Node_T oNNode = NULL;

   assert(pcPath != NULL);
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   iStatus = FT_findNode(pcPath, &oNNode);

   if (iStatus != SUCCESS)
      return NULL;
   

   pvOldContents = Node_replaceContents(oNNode, pvNewContents, ulNewLength);

   /* NEEDED? assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount)); */
   return pvOldContents;


}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
   int iStatus;
   Node_T oNNode = NULL;
   
   assert(pcPath != NULL);
   assert(pbIsFile != NULL);
   assert(pulSize != NULL);

   iStatus = FT_findNode(pcPath, &oNNode);
   
   if (iStatus != SUCCESS)
      return iStatus;

   if (Node_isFile(oNNode)) {
      *pbIsFile = TRUE;
      *pulSize = Node_getFileSize(oNNode);
   }
   else {
      *pbIsFile = FALSE;
   }
   return iStatus;

}

int FT_init(void) {
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(bIsInitialized)
      return INITIALIZATION_ERROR;

   bIsInitialized = TRUE;
   oNRoot = NULL;
   ulCount = 0;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}

int FT_destroy(void) {
   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   if(oNRoot) {
      ulCount -= Node_free(oNRoot);
      oNRoot = NULL;
   }

   bIsInitialized = FALSE;

   assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
   return SUCCESS;
}


/* --------------------------------------------------------------------

  The following auxiliary functions are used for generating the
  string representation of the FT.
*/

/*
  Performs a pre-order traversal of the tree rooted at oNRoot,
  inserting each payload to DynArray_T oDynArray beginning at index ulIndex.
  Returns the next unused index in oDynArray after the insertion(s).
*/
static size_t FT_preOrderTraversal(Node_T oNRoot, DynArray_T oDynArray, size_t ulIndex) {
   size_t c;

   assert(oDynArray != NULL);

   if(oNRoot != NULL) {
      (void) DynArray_set(oDynArray, ulIndex, oNRoot);
      ulIndex++;
      for(c = 0; c < Node_getNumChildren(oNRoot); c++) {
         int iStatus;
         Node_T oNChild = NULL;
         iStatus = Node_getChild(oNRoot,c, &oNChild);
         assert(iStatus == SUCCESS);
         ulIndex = FT_preOrderTraversal(oNChild, oDynArray, ulIndex);
      }
   }
   return ulIndex;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}
/*--------------------------------------------------------------------*/

char *FT_toString(void) {
   DynArray_T nodes;
   size_t totalStrlen = 1;
   char *result = NULL;

   if(!bIsInitialized)
      return NULL;

   nodes = DynArray_new(ulCount);
   if (nodes == NULL)
      return NULL;

   (void) FT_preOrderTraversal(oNRoot, nodes, 0);

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                (void*) &totalStrlen);

   result = malloc(totalStrlen);
   if(result == NULL) {
      DynArray_free(nodes);
      return NULL;
   }
   *result = '\0';

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
                (void *) result);

   DynArray_free(nodes);

   return result;
}
