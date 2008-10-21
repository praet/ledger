#if 0
class binary_parser_t : public parser_t
{
 public:
  virtual bool test(std::istream& in) const;

  virtual unsigned int parse(std::istream&	 in,
			     journal_t *	 journal,
			     account_t *	 master        = NULL,
			     const string * original_file = NULL);
};
#endif

static unsigned long       binary_magic_number = 0xFFEED765;
#if defined(DEBUG_ON)
static unsigned long       format_version      = 0x00030000;
#else
static unsigned long       format_version      = 0x00030000;
#endif

#if 0
static account_t **	   accounts;
static account_t **	   accounts_next;
static unsigned int	   account_index;

static commodity_base_t ** base_commodities;
static commodity_base_t ** base_commodities_next;
static unsigned int	   base_commodity_index;

static commodity_t **	   commodities;
static commodity_t **	   commodities_next;
static unsigned int	   commodity_index;

extern char *		   bigints;
extern char *		   bigints_next;
extern unsigned int	   bigints_index;
extern unsigned int	   bigints_count;
#endif

#if 0
inline void read_binary_value(const char *& data, value_t& val)
{
  val.type = static_cast<value_t::type_t>(read_binary_long<int>(data));

  switch (val.type) {
  case value_t::BOOLEAN:
    read_binary_bool(data, *((bool *) val.data));
    break;
  case value_t::INTEGER:
    read_binary_long(data, *((long *) val.data));
    break;
  case value_t::DATETIME:
    read_binary_number(data, *((moment_t *) val.data));
    break;
  case value_t::AMOUNT:
    read_binary_amount(data, *((amount_t *) val.data));
    break;

  case value_t::BALANCE:
  case value_t::BALANCE_PAIR:
    assert(false);
    break;
  }
}

inline void read_binary_mask(const char *& data, mask_t *& mask)
{
  bool exclude;
  read_binary_number(data, exclude);
  string pattern;
  read_binary_string(data, pattern);

  mask = new mask_t(pattern);
  mask->exclude = exclude;
}

inline void read_binary_transaction(const char *& data, transaction_t * xact)
{
  read_binary_number(data, xact->_date);
  read_binary_number(data, xact->_date_eff);
  xact->account = accounts[read_binary_long<account_t::ident_t>(data) - 1];

  unsigned char flag = read_binary_number<unsigned char>(data);
  if (flag == 0) {
    read_binary_amount(data, xact->amount);
  }
  else if (flag == 1) {
    string expr;
    read_binary_string(data, expr);
    xact->amount_expr = expr;

    repitem_t * item =
      repitem_t::wrap(xact, static_cast<repitem_t *>(xact->entry->data));
    xact->data = item;

    xact->amount = valexpr_t(xact->amount_expr).calc(item).amount();
  }

  if (read_binary_bool(data)) {
    xact->cost = new amount_t;
    read_binary_amount(data, *xact->cost);
    read_binary_string(data, xact->cost_expr);
  } else {
    xact->cost = NULL;
  }

  read_binary_number(data, xact->state);
  read_binary_number(data, xact->flags);
  xact->flags |= TRANSACTION_BULK_ALLOC;
  read_binary_string(data, &xact->note);

  xact->beg_pos = read_binary_long<unsigned long>(data);
  read_binary_long(data, xact->beg_line);
  xact->end_pos = read_binary_long<unsigned long>(data);
  read_binary_long(data, xact->end_line);

  xact->data = NULL;
}

inline void read_binary_entry_base(const char *& data, entry_base_t * entry,
				   transaction_t *& xact_pool, bool& finalize)
{
  read_binary_long(data, entry->src_idx);
  entry->beg_pos = read_binary_long<unsigned long>(data);
  read_binary_long(data, entry->beg_line);
  entry->end_pos = read_binary_long<unsigned long>(data);
  read_binary_long(data, entry->end_line);

  bool ignore_calculated = read_binary_bool(data);

  for (unsigned long i = 0, count = read_binary_long<unsigned long>(data);
       i < count;
       i++) {
    new(xact_pool) transaction_t;
    xact_pool->entry = static_cast<entry_t *>(entry);
    read_binary_transaction(data, xact_pool);
    if (ignore_calculated && xact_pool->flags & TRANSACTION_CALCULATED)
      finalize = true;
    entry->add_transaction(xact_pool++);
  }
}

inline void read_binary_entry(const char *& data, entry_t * entry,
			      transaction_t *& xact_pool, bool& finalize)
{
  entry->data =
    repitem_t::wrap(entry, static_cast<repitem_t *>(entry->journal->data));

  read_binary_entry_base(data, entry, xact_pool, finalize);
  read_binary_number(data, entry->_date);
  read_binary_number(data, entry->_date_eff);
  read_binary_string(data, &entry->code);
  read_binary_string(data, &entry->payee);
}

inline void read_binary_auto_entry(const char *& data, auto_entry_t * entry,
				   transaction_t *& xact_pool)
{
  bool ignore;
  read_binary_entry_base(data, entry, xact_pool, ignore);

  string pred_str;
  read_binary_string(data, &pred_str);
  entry->predicate.parse(pred_str);
}

inline void read_binary_period_entry(const char *& data, period_entry_t * entry,
				     transaction_t *& xact_pool, bool& finalize)
{
  read_binary_entry_base(data, entry, xact_pool, finalize);
  read_binary_string(data, &entry->period_string);
  std::istringstream stream(entry->period_string);
  entry->period.parse(stream);
}

inline commodity_base_t * read_binary_commodity_base(const char *& data)
{
  commodity_base_t * commodity = new commodity_base_t;
  *base_commodities_next++ = commodity;

  read_binary_string(data, commodity->symbol);
  read_binary_string(data, commodity->name);
  read_binary_string(data, commodity->note);
  read_binary_number(data, commodity->precision);
  read_binary_number(data, commodity->flags);

  return commodity;
}

inline void read_binary_commodity_base_extra(const char *& data,
					     commodity_t::ident_t ident)
{
  commodity_base_t * commodity = base_commodities[ident];

  bool read_history = false;
  for (unsigned long i = 0, count = read_binary_long<unsigned long>(data);
       i < count;
       i++) {
    moment_t when;
    read_binary_number(data, when);
    amount_t amt;
    read_binary_amount(data, amt);

    // Upon insertion, amt will be copied, which will cause the amount
    // to be duplicated (and thus not lost when the journal's
    // item_pool is deleted).
    if (! commodity->history)
      commodity->history = new commodity_base_t::history_t;
    commodity->history->prices.insert(history_pair(when, amt));

    read_history = true;
  }
  if (read_history)
    read_binary_number(data, commodity->history->last_lookup);

  if (read_binary_bool(data)) {
    amount_t amt;
    read_binary_amount(data, amt);
    commodity->smaller = new amount_t(amt);
  }

  if (read_binary_bool(data)) {
    amount_t amt;
    read_binary_amount(data, amt);
    commodity->larger = new amount_t(amt);
  }
}

inline commodity_t * read_binary_commodity(const char *& data)
{
  commodity_t * commodity = new commodity_t;
  *commodities_next++ = commodity;

  commodity->base =
    base_commodities[read_binary_long<commodity_base_t::ident_t>(data) - 1];

  read_binary_string(data, commodity->qualified_symbol);
  commodity->annotated = false;

  return commodity;
}

inline commodity_t * read_binary_commodity_annotated(const char *& data)
{
  annotated_commodity_t * commodity = new annotated_commodity_t;
  *commodities_next++ = commodity;

  commodity->base =
    base_commodities[read_binary_long<commodity_base_t::ident_t>(data) - 1];

  read_binary_string(data, commodity->qualified_symbol);
  commodity->annotated = true;

  commodity->ptr =
    commodities[read_binary_long<commodity_t::ident_t>(data) - 1];

  // This read-and-then-assign causes a new amount to be allocated
  // which does not live within the bulk allocation pool, since that
  // pool will be deleted *before* the commodities are destroyed.
  amount_t amt;
  read_binary_amount(data, amt);
  commodity->price = amt;

  read_binary_number(data, commodity->date);
  read_binary_string(data, commodity->tag);

  return commodity;
}

inline
account_t * read_binary_account(const char *& data, journal_t * journal,
				account_t * master = NULL)
{
  account_t * acct = new account_t(NULL);
  *accounts_next++ = acct;

  acct->journal = journal;

  account_t::ident_t id;
  read_binary_long(data, id);	// parent id
  if (id == 0xffffffff)
    acct->parent = NULL;
  else
    acct->parent = accounts[id - 1];

  read_binary_string(data, acct->name);
  read_binary_string(data, acct->note);
  read_binary_number(data, acct->depth);

  // If all of the subaccounts will be added to a different master
  // account, throw away what we've learned about the recorded
  // journal's own master account.

  if (master && acct != master) {
    checked_delete(acct);
    acct = master;
  }

  for (account_t::ident_t i = 0,
	 count = read_binary_long<account_t::ident_t>(data);
       i < count;
       i++) {
    account_t * child = read_binary_account(data, journal);
    child->parent = acct;
    assert(acct != child);
    acct->add_account(child);
  }

  return acct;
}

unsigned int read_binary_journal(std::istream&	   in,
				journal_t *	   journal,
				account_t *	   master,
				const string& original_file)
{
  account_index	       =
  base_commodity_index =
  commodity_index      = 0;

  // Read in the files that participated in this journal, so that they
  // can be checked for changes on reading.

  if (! original_file.empty()) {
    for (unsigned short i = 0,
	   count = read_binary_number<unsigned short>(in);
	 i < count;
	 i++) {
      string path = read_binary_string(in);
      std::time_t old_mtime;
      read_binary_number(in, old_mtime);
      struct stat info;
      stat(path.c_str(), &info);
      if (std::difftime(info.st_mtime, old_mtime) > 0)
	return 0;

      journal->sources.push_back(path);
    }

    // Make sure that the cache uses the same price database,
    // otherwise it means that LEDGER_PRICE_DB has been changed, and
    // we should ignore this cache file.
    if (read_binary_string(in) != journal->price_db)
      return 0;
  }

  // Read all of the data in at once, so that we're just dealing with
  // a big data buffer.

  unsigned long data_size = read_binary_number<unsigned long>(in);

  char * data_pool = new char[data_size];
  char * data = data_pool;
  in.read(data, data_size);

  // Read in the accounts

  account_t::ident_t a_count = read_binary_long<account_t::ident_t>(data);
  accounts = accounts_next = new account_t *[a_count];

  assert(journal->master);
  checked_delete(journal->master);
  journal->master = read_binary_account(data, journal, master);

  if (read_binary_bool(data))
    journal->basket = accounts[read_binary_long<account_t::ident_t>(data) - 1];

  // Allocate the memory needed for the entries and transactions in
  // one large block, which is then chopped up and custom constructed
  // as necessary.

  unsigned long count        = read_binary_long<unsigned long>(data);
  unsigned long auto_count   = read_binary_long<unsigned long>(data);
  unsigned long period_count = read_binary_long<unsigned long>(data);
  unsigned long xact_count   = read_binary_number<unsigned long>(data);
  unsigned long bigint_count = read_binary_number<unsigned long>(data);

  std::size_t pool_size = (sizeof(entry_t) * count +
			   sizeof(transaction_t) * xact_count +
			   sizeof_bigint_t() * bigint_count);

  char * item_pool = new char[pool_size];

  journal->item_pool	 = item_pool;
  journal->item_pool_end = item_pool + pool_size;

  entry_t *	  entry_pool = (entry_t *) item_pool;
  transaction_t * xact_pool  = (transaction_t *) (item_pool +
						  sizeof(entry_t) * count);
  bigints_index = 0;
  bigints = bigints_next = (item_pool + sizeof(entry_t) * count +
			    sizeof(transaction_t) * xact_count);

  // Read in the base commodities and then derived commodities

  commodity_base_t::ident_t bc_count =
    read_binary_long<commodity_base_t::ident_t>(data);
  base_commodities = base_commodities_next = new commodity_base_t *[bc_count];

  for (commodity_base_t::ident_t i = 0; i < bc_count; i++) {
    commodity_base_t * commodity = read_binary_commodity_base(data);

    std::pair<base_commodities_map::iterator, bool> result =
      commodity_base_t::commodities.insert
      (base_commodities_pair(commodity->symbol, commodity));
    if (! result.second) {
      base_commodities_map::iterator c =
	commodity_base_t::commodities.find(commodity->symbol);

      // It's possible the user might have used a commodity in a value
      // expression passed to an option, we'll just override the
      // flags, but keep the commodity pointer intact.
      if (c == commodity_base_t::commodities.end())
	throw new error(string("Failed to read base commodity from cache: ") +
			commodity->symbol);

      (*c).second->name	     = commodity->name;
      (*c).second->note	     = commodity->note;
      (*c).second->precision = commodity->precision;
      (*c).second->flags     = commodity->flags;
      if ((*c).second->smaller)
	checked_delete((*c).second->smaller);
      (*c).second->smaller   = commodity->smaller;
      if ((*c).second->larger)
	checked_delete((*c).second->larger);
      (*c).second->larger    = commodity->larger;

      *(base_commodities_next - 1) = (*c).second;
      checked_delete(commodity);
    }
  }

  commodity_t::ident_t c_count  = read_binary_long<commodity_t::ident_t>(data);
  commodities = commodities_next = new commodity_t *[c_count];

  for (commodity_t::ident_t i = 0; i < c_count; i++) {
    commodity_t * commodity;
    string   mapping_key;

    if (! read_binary_bool(data)) {
      commodity	  = read_binary_commodity(data);
      mapping_key = commodity->base->symbol;
    } else {
      read_binary_string(data, mapping_key);
      commodity = read_binary_commodity_annotated(data);
    }

    std::pair<commodities_map::iterator, bool> result =
      commodity_t::commodities.insert(commodities_pair
					(mapping_key, commodity));
    if (! result.second) {
      commodities_map::iterator c =
	commodity_t::commodities.find(mapping_key);
      if (c == commodity_t::commodities.end())
	throw new error(string("Failed to read commodity from cache: ") +
			commodity->symbol());

      *(commodities_next - 1) = (*c).second;
      checked_delete(commodity);
    }
  }

  for (commodity_base_t::ident_t i = 0; i < bc_count; i++)
    read_binary_commodity_base_extra(data, i);

  commodity_t::ident_t ident;
  read_binary_long(data, ident);
  if (ident == 0xffffffff || ident == 0)
    commodity_t::default_commodity = NULL;
  else
    commodity_t::default_commodity = commodities[ident - 1];

  // Read in the entries and transactions

  for (unsigned long i = 0; i < count; i++) {
    new(entry_pool) entry_t;
    bool finalize = false;
    entry_pool->journal = journal;
    read_binary_entry(data, entry_pool, xact_pool, finalize);
    if (finalize && ! entry_pool->finalize())
      continue;
    journal->entries.push_back(entry_pool++);
  }

  for (unsigned long i = 0; i < auto_count; i++) {
    auto_entry_t * auto_entry = new auto_entry_t;
    read_binary_auto_entry(data, auto_entry, xact_pool);
    auto_entry->journal = journal;
    journal->auto_entries.push_back(auto_entry);
  }

  for (unsigned long i = 0; i < period_count; i++) {
    period_entry_t * period_entry = new period_entry_t;
    bool finalize = false;
    read_binary_period_entry(data, period_entry, xact_pool, finalize);
    period_entry->journal = journal;
    if (finalize && ! period_entry->finalize())
      continue;
    journal->period_entries.push_back(period_entry);
  }

  // Clean up and return the number of entries read

  checked_array_delete(accounts);
  checked_array_delete(commodities);
  checked_array_delete(data_pool);

  VALIDATE(journal->valid());

  return count;
}
#endif

#if 0
bool binary_parser_t::test(std::istream& in) const
{
  if (read_binary_number_nocheck<unsigned long>(in) == binary_magic_number &&
      read_binary_number_nocheck<unsigned long>(in) == format_version)
    return true;

  in.clear();
  in.seekg(0, std::ios::beg);
  return false;
}

unsigned int binary_parser_t::parse(std::istream&       in,
				    journal_t *		journal,
				    account_t *		master,
				    const string * original_file)
{
#if 0
  return read_binary_journal(in, journal, master,
			     original_file ? *original_file : "");
#endif
}
#endif


#if 0
void write_binary_value(std::ostream& out, const value_t& val)
{
  write_binary_long(out, (int)val.type);

  switch (val.type) {
  case value_t::BOOLEAN:
    write_binary_bool(out, *((bool *) val.data));
    break;
  case value_t::INTEGER:
    write_binary_long(out, *((long *) val.data));
    break;
  case value_t::DATETIME:
    write_binary_number(out, *((moment_t *) val.data));
    break;
  case value_t::AMOUNT:
    write_binary_amount(out, *((amount_t *) val.data));
    break;

  case value_t::BALANCE:
  case value_t::BALANCE_PAIR:
    throw new error("Cannot write a balance to the binary cache");
  }
}

void write_binary_mask(std::ostream& out, mask_t * mask)
{
  write_binary_number(out, mask->exclude);
  write_binary_string(out, mask->pattern);
}

void write_binary_transaction(std::ostream& out, transaction_t * xact,
			      bool ignore_calculated)
{
  write_binary_number(out, xact->_date);
  write_binary_number(out, xact->_date_eff);
  write_binary_long(out, xact->account->ident);

  if (ignore_calculated && xact->flags & TRANSACTION_CALCULATED) {
    write_binary_number<unsigned char>(out, 0);
    write_binary_amount(out, amount_t());
  }
  else if (! xact->amount_expr.empty()) {
    write_binary_number<unsigned char>(out, 1);
    write_binary_string(out, xact->amount_expr);
  }
  else {
    write_binary_number<unsigned char>(out, 0);
    write_binary_amount(out, xact->amount);
  }

  if (xact->cost &&
      (! (ignore_calculated && xact->flags & TRANSACTION_CALCULATED))) {
    write_binary_bool(out, true);
    write_binary_amount(out, *xact->cost);
    write_binary_string(out, xact->cost_expr);
  } else {
    write_binary_bool(out, false);
  }

  write_binary_number(out, xact->state);
  write_binary_number(out, xact->flags);
  write_binary_string(out, xact->note);

  write_binary_long(out, xact->beg_pos);
  write_binary_long(out, xact->beg_line);
  write_binary_long(out, xact->end_pos);
  write_binary_long(out, xact->end_line);
}

void write_binary_entry_base(std::ostream& out, entry_base_t * entry)
{
  write_binary_long(out, entry->src_idx);
  write_binary_long(out, entry->beg_pos);
  write_binary_long(out, entry->beg_line);
  write_binary_long(out, entry->end_pos);
  write_binary_long(out, entry->end_line);

  bool ignore_calculated = false;
  for (transactions_list::const_iterator i = entry->transactions.begin();
       i != entry->transactions.end();
       i++)
    if (! (*i)->amount_expr.empty()) {
      ignore_calculated = true;
      break;
    }

  write_binary_bool(out, ignore_calculated);

  write_binary_long(out, entry->transactions.size());
  for (transactions_list::const_iterator i = entry->transactions.begin();
       i != entry->transactions.end();
       i++)
    write_binary_transaction(out, *i, ignore_calculated);
}

void write_binary_entry(std::ostream& out, entry_t * entry)
{
  write_binary_entry_base(out, entry);
  write_binary_number(out, entry->_date);
  write_binary_number(out, entry->_date_eff);
  write_binary_string(out, entry->code);
  write_binary_string(out, entry->payee);
}

void write_binary_auto_entry(std::ostream& out, auto_entry_t * entry)
{
  write_binary_entry_base(out, entry);
  write_binary_string(out, entry->predicate.expr);
}

void write_binary_period_entry(std::ostream& out, period_entry_t * entry)
{
  write_binary_entry_base(out, entry);
  write_binary_string(out, entry->period_string);
}

void write_binary_commodity_base(std::ostream& out, commodity_base_t * commodity)
{
  commodity->ident = ++base_commodity_index;

  write_binary_string(out, commodity->symbol);
  write_binary_string(out, commodity->name);
  write_binary_string(out, commodity->note);
  write_binary_number(out, commodity->precision);
  write_binary_number(out, commodity->flags);
}

void write_binary_commodity_base_extra(std::ostream& out,
				       commodity_base_t * commodity)
{
  if (commodity->history && commodity->history->bogus_time)
    commodity->remove_price(commodity->history->bogus_time);

  if (! commodity->history) {
    write_binary_long<unsigned long>(out, 0);
  } else {
    write_binary_long<unsigned long>(out, commodity->history->prices.size());
    for (history_map::const_iterator i = commodity->history->prices.begin();
	 i != commodity->history->prices.end();
	 i++) {
      write_binary_number(out, (*i).first);
      write_binary_amount(out, (*i).second);
    }
    write_binary_number(out, commodity->history->last_lookup);
  }

  if (commodity->smaller) {
    write_binary_bool(out, true);
    write_binary_amount(out, *commodity->smaller);
  } else {
    write_binary_bool(out, false);
  }

  if (commodity->larger) {
    write_binary_bool(out, true);
    write_binary_amount(out, *commodity->larger);
  } else {
    write_binary_bool(out, false);
  }
}

void write_binary_commodity(std::ostream& out, commodity_t * commodity)
{
  commodity->ident = ++commodity_index;

  write_binary_long(out, commodity->base->ident);
  write_binary_string(out, commodity->qualified_symbol);
}

void write_binary_commodity_annotated(std::ostream& out,
				      commodity_t * commodity)
{
  commodity->ident = ++commodity_index;

  write_binary_long(out, commodity->base->ident);
  write_binary_string(out, commodity->qualified_symbol);

  annotated_commodity_t * ann_comm =
    static_cast<annotated_commodity_t *>(commodity);

  write_binary_long(out, ann_comm->base->ident);
  write_binary_amount(out, ann_comm->price);
  write_binary_number(out, ann_comm->date);
  write_binary_string(out, ann_comm->tag);
}

static inline account_t::ident_t count_accounts(account_t * account)
{
  account_t::ident_t count = 1;

  for (accounts_map::iterator i = account->accounts.begin();
       i != account->accounts.end();
       i++)
    count += count_accounts((*i).second);

  return count;
}

void write_binary_account(std::ostream& out, account_t * account)
{
  account->ident = ++account_index;

  if (account->parent)
    write_binary_long(out, account->parent->ident);
  else
    write_binary_long<account_t::ident_t>(out, 0xffffffff);

  write_binary_string(out, account->name);
  write_binary_string(out, account->note);
  write_binary_number(out, account->depth);

  write_binary_long<account_t::ident_t>(out, account->accounts.size());
  for (accounts_map::iterator i = account->accounts.begin();
       i != account->accounts.end();
       i++)
    write_binary_account(out, (*i).second);
}

void write_binary_journal(std::ostream& out, journal_t * journal)
{
  account_index	       =
  base_commodity_index =
  commodity_index      = 0;

  write_binary_number_nocheck(out, binary_magic_number);
  write_binary_number_nocheck(out, format_version);

  // Write out the files that participated in this journal, so that
  // they can be checked for changes on reading.

  if (journal->sources.empty()) {
    write_binary_number<unsigned short>(out, 0);
  } else {
    write_binary_number<unsigned short>(out, journal->sources.size());
    for (strings_list::const_iterator i = journal->sources.begin();
	 i != journal->sources.end();
	 i++) {
      write_binary_string(out, *i);
      struct stat info;
      stat((*i).c_str(), &info);
      write_binary_number(out, std::time_t(info.st_mtime));
    }

    // Write out the price database that relates to this data file, so
    // that if it ever changes the cache can be invalidated.
    write_binary_string(out, journal->price_db);
  }

  ostream_pos_type data_val = out.tellp();
  write_binary_number<unsigned long>(out, 0);

  // Write out the accounts

  write_binary_long<account_t::ident_t>(out, count_accounts(journal->master));
  write_binary_account(out, journal->master);

  if (journal->basket) {
    write_binary_bool(out, true);
    write_binary_long(out, journal->basket->ident);
  } else {
    write_binary_bool(out, false);
  }

  // Write out the number of entries, transactions, and amounts

  write_binary_long<unsigned long>(out, journal->entries.size());
  write_binary_long<unsigned long>(out, journal->auto_entries.size());
  write_binary_long<unsigned long>(out, journal->period_entries.size());

  ostream_pos_type xacts_val = out.tellp();
  write_binary_number<unsigned long>(out, 0);

  ostream_pos_type bigints_val = out.tellp();
  write_binary_number<unsigned long>(out, 0);

  bigints_count = 0;

  // Write out the commodities

  write_binary_long<commodity_t::ident_t>
    (out, commodity_base_t::commodities.size());

  for (base_commodities_map::const_iterator i =
	 commodity_base_t::commodities.begin();
       i != commodity_base_t::commodities.end();
       i++)
    write_binary_commodity_base(out, (*i).second);

  write_binary_long<commodity_t::ident_t>
    (out, commodity_t::commodities.size());

  for (commodities_map::const_iterator i = commodity_t::commodities.begin();
       i != commodity_t::commodities.end();
       i++) {
    if (! (*i).second->annotated) {
      write_binary_bool(out, false);
      write_binary_commodity(out, (*i).second);
    }
  }

  for (commodities_map::const_iterator i = commodity_t::commodities.begin();
       i != commodity_t::commodities.end();
       i++) {
    if ((*i).second->annotated) {
      write_binary_bool(out, true);
      write_binary_string(out, (*i).first); // the mapping key
      write_binary_commodity_annotated(out, (*i).second);
    }
  }

  // Write out the history and smaller/larger convertible links after
  // both the base and the main commodities have been written, since
  // the amounts in both will refer to the mains.

  for (base_commodities_map::const_iterator i =
	 commodity_base_t::commodities.begin();
       i != commodity_base_t::commodities.end();
       i++)
    write_binary_commodity_base_extra(out, (*i).second);

  if (commodity_t::default_commodity)
    write_binary_long(out, commodity_t::default_commodity->ident);
  else
    write_binary_long<commodity_t::ident_t>(out, 0xffffffff);

  // Write out the entries and transactions

  unsigned long xact_count = 0;

  for (entries_list::const_iterator i = journal->entries.begin();
       i != journal->entries.end();
       i++) {
    write_binary_entry(out, *i);
    xact_count += (*i)->transactions.size();
  }

  for (auto_entries_list::const_iterator i = journal->auto_entries.begin();
       i != journal->auto_entries.end();
       i++) {
    write_binary_auto_entry(out, *i);
    xact_count += (*i)->transactions.size();
  }

  for (period_entries_list::const_iterator i = journal->period_entries.begin();
       i != journal->period_entries.end();
       i++) {
    write_binary_period_entry(out, *i);
    xact_count += (*i)->transactions.size();
  }

  // Back-patch the count for amounts

  unsigned long data_size = (((unsigned long) out.tellp()) -
			     ((unsigned long) data_val) -
			     sizeof(unsigned long));
  out.seekp(data_val);
  write_binary_number<unsigned long>(out, data_size);
  out.seekp(xacts_val);
  write_binary_number<unsigned long>(out, xact_count);
  out.seekp(bigints_val);
  write_binary_number<unsigned long>(out, bigints_count);
}
#endif
