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
   Node_T oNParent;
   Node_T *oNSibling;
   Path_T oPNPath;
   Path_T oPPPath;
   size_t ulDepth;
   size_t i;

   ulDepth = Path_getDepth(oPNPath);

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
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
   oPNPath = Node_getPath(oNNode);
   if (Node_getPath(oNNode) == NULL) {
      fprintf(stderr, "There is a node that has a NULL path\n");
      return FALSE;
   }

   /* Node cannot have the same name as any of its siblings */
   i = 0;
   while (i < Node_getNumChildren(oNParent)) {
      int numberOfEquivalences;
      numberOfEquivalences = 0;
      Node_getChild(oNParent, i, oNSibling);
      if (Node_compare(oNNode, *oNSibling)) {
         numberOfEquivalences++;
         if (numberOfEquivalences > 1) {
            fprintf(stderr, "Two siblings have the same name\n");
            return FALSE; 
         }
      }
      i++;
   }

   /* The root node should not contain a backward slash */
   if (ulDepth == 1 && strchr(Path_getPathname(oNNode), '/')) {
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
static boolean CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;

   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;

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
         if(!CheckerDT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if (oNRoot != NULL) {
         fprintf(stderr, "Not initialized, but oNRoot is not NULL");
         return FALSE;
      }
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }

   if (oNRoot && ulCount != Node_getNumChildren + 1) {
      fprintf(stderr, "Length of the path is not equal to one more than the number of children\n");
      return FALSE;
   }

   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
  
}
