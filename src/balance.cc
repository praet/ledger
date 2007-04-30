#include "balance.h"

namespace ledger {

amount_t balance_t::amount(const commodity_t& commodity) const
{
  if (! commodity) {
    if (amounts.size() == 1) {
      amounts_map::const_iterator i = amounts.begin();
      return (*i).second;
    }
    else if (amounts.size() > 1) {
      // Try stripping annotations before giving an error.
      balance_t temp(strip_annotations());
      if (temp.amounts.size() == 1)
	return temp.amount(commodity);

      throw_(amount_exception,
	     "Requested amount of a balance with multiple commodities: " << temp);
    }
  }
  else if (amounts.size() > 0) {
    amounts_map::const_iterator i = amounts.find(&commodity);
    if (i != amounts.end())
      return (*i).second;
  }
  return amount_t();
}

balance_t balance_t::value(const moment_t& moment) const
{
  balance_t temp;

  for (amounts_map::const_iterator i = amounts.begin();
       i != amounts.end();
       i++)
    temp += (*i).second.value(moment);

  return temp;
}

balance_t balance_t::price() const
{
  balance_t temp;

  for (amounts_map::const_iterator i = amounts.begin();
       i != amounts.end();
       i++)
    temp += (*i).second.price();

  return temp;
}

moment_t balance_t::date() const
{
  moment_t temp;

  for (amounts_map::const_iterator i = amounts.begin();
       i != amounts.end();
       i++) {
    moment_t tdate = (*i).second.date();
    if (! is_valid_moment(temp) && is_valid_moment(tdate))
      temp = tdate;
    else if (temp != tdate)
      return moment_t();
  }
  return temp;
}

balance_t balance_t::strip_annotations(const bool keep_price,
				       const bool keep_date,
				       const bool keep_tag) const
{
  balance_t temp;

  for (amounts_map::const_iterator i = amounts.begin();
       i != amounts.end();
       i++)
    temp += (*i).second.strip_annotations(keep_price, keep_date, keep_tag);

  return temp;
}

void balance_t::write(std::ostream& out,
		      const int     first_width,
		      const int     latter_width) const
{
  bool first  = true;
  int  lwidth = latter_width;

  if (lwidth == -1)
    lwidth = first_width;

  if (commodity_t::commodities_sorted) {
    for (amounts_map::const_iterator i = amounts.begin();
	 i != amounts.end();
	 i++) {
      int width;
      if (! first) {
	out << std::endl;
	width = lwidth;
      } else {
	first = false;
	width = first_width;
      }

      out.width(width);
      out.fill(' ');
      out << std::right << (*i).second;
    }
  } else {
    typedef std::vector<const amount_t *> amounts_array;
    amounts_array sorted;

    for (amounts_map::const_iterator i = amounts.begin();
	 i != amounts.end();
	 i++)
      if ((*i).second)
	sorted.push_back(&(*i).second);

    std::stable_sort(sorted.begin(), sorted.end(),
		     compare_amount_commodities());

    for (amounts_array::const_iterator i = sorted.begin();
	 i != sorted.end();
	 i++) {
      int width;
      if (! first) {
	out << std::endl;
	width = lwidth;
      } else {
	first = false;
	width = first_width;
      }

      out.width(width);
      out.fill(' ');
      out << std::right << **i;
    }
  }

  if (first) {
    out.width(first_width);
    out.fill(' ');
    out << std::right << "0";
  }
}

balance_t& balance_t::operator*=(const balance_t& bal)
{
  if (realzero() || bal.realzero()) {
    return *this = 0L;
  }
  else if (bal.amounts.size() == 1) {
    return *this *= (*bal.amounts.begin()).second;
  }
  else if (amounts.size() == 1) {
    return *this = bal * *this;
  }
  else {
    // Since we would fail with an error at this point otherwise, try
    // stripping annotations to see if we can come up with a
    // reasonable result.  The user will not notice any annotations
    // missing (since they are viewing a stripped report anyway), only
    // that some of their value expression may not see any pricing or
    // date data because of this operation.

    balance_t temp(bal.strip_annotations());
    if (temp.amounts.size() == 1)
      return *this *= temp;
    temp = strip_annotations();
    if (temp.amounts.size() == 1)
      return *this = bal * temp;

    std::ostringstream errmsg;
    errmsg << "Cannot multiply two balances: " << temp << " * " << bal;
    throw amount_exception(errmsg.str(), context());
  }
}

balance_t& balance_t::operator*=(const amount_t& amt)
{
  if (realzero() || amt.realzero()) {
    return *this = 0L;
  }
  else if (! amt.commodity()) {
    // Multiplying by the null commodity causes all amounts to be
    // increased by the same factor.
    for (amounts_map::iterator i = amounts.begin();
	 i != amounts.end();
	 i++)
      (*i).second *= amt;
  }
  else if (amounts.size() == 1) {
    *this = (*amounts.begin()).second * amt;
  }
  else {
    amounts_map::iterator i = amounts.find(&amt.commodity());
    if (i != amounts.end()) {
      (*i).second *= amt;
    } else {
      // Try stripping annotations before giving an error.
      balance_t temp(strip_annotations());
      if (temp.amounts.size() == 1) {
	return *this = (*temp.amounts.begin()).second * amt;
      } else {
	i = temp.amounts.find(&amt.commodity());
	if (i != temp.amounts.end())
	  return *this = temp * amt;
      }

      std::ostringstream errmsg;
      errmsg << "Attempt to multiply balance by a commodity"
	     << " not found in that balance: "
	     << temp << " * " << amt;
      throw amount_exception(errmsg.str(), context());
    }
  }
  return *this;
}

balance_t& balance_t::operator/=(const balance_t& bal)
{
  if (bal.realzero()) {
    std::ostringstream errmsg;
    errmsg << "Attempt to divide by zero: " << *this << " / " << bal;
    throw amount_exception(errmsg.str(), context());
  }
  else if (realzero()) {
    return *this = 0L;
  }
  else if (bal.amounts.size() == 1) {
    return *this /= (*bal.amounts.begin()).second;
  }
  else if (*this == bal) {
    return *this = 1L;
  }
  else {
    // Try stripping annotations before giving an error.
    balance_t temp(bal.strip_annotations());
    if (temp.amounts.size() == 1)
      return *this /= temp;

    std::ostringstream errmsg;
    errmsg << "Cannot divide between two balances: " << temp << " / " << bal;
    throw amount_exception(errmsg.str(), context());
  }
}

balance_t& balance_t::operator/=(const amount_t& amt)
{
  if (amt.realzero()) {
    std::ostringstream errmsg;
    errmsg << "Attempt to divide by zero: " << *this << " / " << amt;
    throw amount_exception(errmsg.str(), context());
  }
  else if (realzero()) {
    return *this = 0L;
  }
  else if (! amt.commodity()) {
    // Dividing by the null commodity causes all amounts to be
    // decreased by the same factor.
    for (amounts_map::iterator i = amounts.begin();
	 i != amounts.end();
	 i++)
      (*i).second /= amt;
  }
  else if (amounts.size() == 1 &&
	   (*amounts.begin()).first == &amt.commodity()) {
    (*amounts.begin()).second /= amt;
  }
  else {
    amounts_map::iterator i = amounts.find(&amt.commodity());
    if (i != amounts.end()) {
      (*i).second /= amt;
    } else {
      // Try stripping annotations before giving an error.
      balance_t temp(strip_annotations());
      if (temp.amounts.size() == 1 &&
	  (*temp.amounts.begin()).first == &amt.commodity())
	return *this = temp / amt;

      std::ostringstream errmsg;
      errmsg << "Attempt to divide balance by a commodity"
	     << " not found in that balance: "
	     << temp << " * " << amt;
      throw amount_exception(errmsg.str(), context());
    }
  }
  return *this;
}

balance_t::operator amount_t() const
{
  if (amounts.size() == 1) {
    return (*amounts.begin()).second;
  }
  else if (amounts.size() == 0) {
    return amount_t();
  }
  else {
    // Try stripping annotations before giving an error.
    balance_t temp(strip_annotations());
    if (temp.amounts.size() == 1)
      return (*temp.amounts.begin()).second;

    throw_(amount_exception,
	   "Cannot convert a balance with " <<
	   "multiple commodities to an amount: " << temp);
  }
}

} // namespace ledger