/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：list.h
	描述		：本文件定义了通用链表服务
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

#ifndef _LIST_H
#define _LIST_H

//链表头定义
struct list_head {
	struct list_head *next, *prev;
};


/*************************************************
  链表节点初始化
*************************************************/
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}


/*************************************************
  链表节点添加功能
*************************************************/
//添加一个节点new到两个连续的节点prev和next之间
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
//在节点head之后添加一个节点new
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
//在节点head之前添加一个节点new
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}



/*************************************************
  链表节点删除功能
*************************************************/
//把两个节点prev和next互相连接，即从链表中删除这两个节点之间的所有节点
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}
//删除一个节点entry，并置于未初始化状态
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}
//删除一个节点entry，并重新初始化
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}



/*************************************************
  链表节点替换功能
*************************************************/
//用节点new替换节点old，并不修改节点old的值
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}
//用节点new替换节点old，并重新初始化节点old
static inline void list_replace_init(struct list_head *old,
					struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}


/*************************************************
  链表节点移动功能
*************************************************/
//移动一个节点list到head之前
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}
//移动一个节点list到head之后
static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}




/*************************************************
  链表节点判断功能
*************************************************/
//判断list节点是不是head链表的最后一个
static inline int list_is_last(const struct list_head *list,
				const struct list_head *head)
{
	return list->next == head;
}
//判断head节点是不是空闲
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}
//线程安全判断head节点是不是空闲
static inline int list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}
//判断head链表是不是只含有一个节点
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}



/*************************************************
  链表分割功能
*************************************************/
//用于被list_cut_position调用
static inline void __list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	struct list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}
//将一个链表分成两个链表。list是新产生的链表，head是原有的链表，entry之后的归head，之前（包括entry）归list。
static inline void list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}



/*************************************************
  链表合并功能
*************************************************/
//用于被list_splice调用
static inline void __list_splice(const struct list_head *list,
				 struct list_head *prev,
				 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}
//在head节点之后加入list链表
static inline void list_splice(const struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}
//在head节点之前加入list链表
static inline void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}
//在head节点之后加入list链表，并将list链表重新初始化
static inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head, head->next);
		INIT_LIST_HEAD(list);
	}
}
//在head节点之前加入list链表，并将list链表重新初始化
static inline void list_splice_tail_init(struct list_head *list,
					 struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head->prev, head);
		INIT_LIST_HEAD(list);
	}
}



/*************************************************
  链表遍历功能
*************************************************/
#define offsetof(TYPE, MEMBER) ((u32) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

//获得包含节点的结构
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
//获得此链表的第一个节点结构
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)
//正向遍历链表head节点
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)
//反向遍历链表head节点
#define list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)
//正向遍历链表head所在的结构
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
//反向遍历链表head所在的结构
#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))
//从pos结构开始正向遍历链表所在的结构，不包括pos本身
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
//从pos结构开始反向遍历链表所在的结构，不包括pos本身
#define list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))
//从pos结构开始反向遍历链表所在的结构，包括pos本身
#define list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head); pos = list_entry(pos->member.next, typeof(*pos), member))


#endif
