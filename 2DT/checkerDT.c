/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent = NULL;
   Node_T oNSibling = NULL;
   Path_T oPNPath;
   Path_T oPPPath;
   size_t ulDepth;
   size_t i;
   int numberOfEquivalences;
   size_t childID = 0;

   oPNPath = Node_getPath(oNNode);
   ulDepth = Path_getDepth(oPNPath);
   oNParent = Node_getParent(oNNode);


   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   /* The root node should not have a parent */
   if (ulDepth == 1 && oNParent != NULL) {
      fprintf(stderr, "The root node has a parent\n");
      return FALSE;
   }


   /* If it is not the root, it must have a parent */
   if (ulDepth > 1 && oNParent == NULL) {
      fprintf(stderr, "There is a node that does not have a parent\n");
      return FALSE;
   }

   /* oPPathshould not equal NULL */
   if (oPNPath == NULL) {
      fprintf(stderr, "There is a node that has a NULL path\n");
      return FALSE;
   }

   /* Node cannot have the same name as any of its siblings */
   /* Nodes should be in alphabetical order */
   if (ulDepth > 1) {
      i = 0;
      numberOfEquivalences = 0;
      while (i < Node_getNumChildren(oNParent)) {
         Node_getChild(oNParent, i, &oNSibling);
         if (oNSibling != NULL) {
            int siblingComparison;
            siblingComparison = Node_compare(oNNode, oNSibling);
            if (!siblingComparison) {
               numberOfEquivalences++;
               if (numberOfEquivalences > 1) {
                  fprintf(stderr, "Two siblings have the same name\n");
                  return FALSE; 
               }
            }
         }
         i++;
      }
   }

   if (ulDepth > 1)
   {
      int i = 1;
      int j = 0;
      while (i < Node_getNumChildren(oNParent))
      {
         Node_T iNode = NULL;
         Node_T iminus1Node = NULL;
         Node_getChild(oNParent, i, &iNode);
         Node_getChild(oNParent, i-1, &iminus1Node);

         if (iNode && iminus1Node)
         {
            int siblingComparison;
            siblingComparison = Node_compare(iNode, iminus1Node);
            if (siblingComparison < 0)
            {
               fprintf(stderr, "Two siblings in incorrect order\n");
               return FALSE; 
            }
         }

         i++;
      }
   }

   /* The root node should not contain a backward slash */
   if (ulDepth == 1 && strchr(Path_getPathname(Node_getPath(oNNode)), '/')) {
      fprintf(stderr, "Root node should not contain a backward slash\n");
      return FALSE;
   }
   
   return TRUE;
}


/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode, size_t ulCount) {
   size_t ulIndex;
   static size_t checkerCount = 0;
   
   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;
      checkerCount++;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild, 0))
            return FALSE;
      }
   }

   if (ulCount)
   {
      if (checkerCount != ulCount)
      {
         fprintf(stderr, 
         "checkerCount=%lu does not match stored count value=%lu\n",
             checkerCount, ulCount);
         return FALSE;
      }
      checkerCount = 0;
   }

   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized)
   {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
      /* Similarly, if DT is not initialized, root should be NULL */
      if(oNRoot != NULL)
      {
         fprintf(stderr, "Not initialized, but root is not NULL\n");
         return FALSE;
      }
      return TRUE;
   }
   else
   {
      /* If ulCount > 0, then oNRoot should not be NULL */
      if(ulCount && oNRoot == NULL) {
         fprintf(stderr, "Initialized, but ulCount>0 + oNRoot=NULL");
         return FALSE;
      }
      /* If ulCount == 0, then oNRoot should be NULL */
      else if (!ulCount && oNRoot){
         fprintf(stderr, "Initialized, but ulcount=0 + oNRoot!=NULL");
      }
   }

   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot, ulCount);
   
}
