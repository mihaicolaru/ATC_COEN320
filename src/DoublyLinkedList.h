/*
 * DoublyLinkedList.h
 *
 *  Created on: Nov. 5, 2022
 *      Author: Mihai
 */

#ifndef DOUBLYLINKEDLIST_H_
#define DOUBLYLINKEDLIST_H_

#include <iostream>

#include "Plane.h"

class Node {
public:
	Plane* plane;
	Node* next;
	Node* prev;
};


class DoublyLinkedList{
public:
	DoublyLinkedList(){
		Node* head = NULL;
		headRef = &head;
	}

	~DoublyLinkedList(){

	}

	void push(Plane** newPlane){
		// allocate node
		Node* newNode = new Node();

		// put in new plane pointer
		newNode->plane = (*newPlane);

		// adding at beginning, prev always null
		newNode->prev = NULL;

		// link rest of list
		newNode->next = (*headRef);

		// change prev of head node to new node
		if((*headRef) != NULL){
			(*headRef)->prev = newNode;
		}

		// move head to point to new node
		(*headRef) = newNode;
	}

	void deleteNode(Node* del){
		// base case
		if(*headRef == NULL || del == NULL){
			return;
		}

		// if node to be deleted is head node
		if(*headRef == del){
			*headRef = del->next;
		}

		// change next only if node to be deleted is NOT the last one
		if(del->next != NULL){
			del->next->prev = del->prev;
		}

		// change prev only if node to be deleted is NOT the first one
		if(del->prev != NULL){
			del->prev->next = del->next;
		}

		free(del);
		return;
	}

private:
	Node** headRef;	// pointer to head of queue
	Node** currentRef;	// pointer to current node
};


#endif /* DOUBLYLINKEDLIST_H_ */
