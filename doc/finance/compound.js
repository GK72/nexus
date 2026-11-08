// Model of `compound.adoc`, for the interactive block of `tools/asciidoc/plugins/interactive.rb` (inputs in
// `compound.rb`).

(() => {
  const annuity = (principal, rate, periods) => {
    if (principal <= 0 || periods <= 0) return 0;
    if (rate === 0) return principal / periods;
    return (principal * rate) / (1 - (1 + rate) ** -periods);
  };

  // Savings, cash and a loan, month by month. The salary rises by `raise` at the start of every
  // year; each month the expenses and the deposit go out of it. The savings (starting balance and
  // deposits) earn a twelfth of the yearly rate a month; the cash (what is left of the salary)
  // earns nothing and pays the loan, a monthly annuity. The columns are the savings with the loan
  // still owed below zero; the line is everything with the loan: savings + cash, in today's money
  // (divided by the inflation since the start, compounded monthly) when `real` is checked.
  NxsInteractive.model('compound', ({ balance, rate, inflation = 0, salary = 0, raise = 0, expenses = 0, deposit = 0, loan = 0, loan_rate = 0, term = 0, years, real = false, unit }) => {
    const months = Math.max(1, Math.round(years * 12));
    const periods = Math.round(term * 12);
    const loanMonthly = loan_rate / 12;
    const payment = annuity(loan, loanMonthly, periods);
    const keys = ['xs', 'deposits', 'earned', 'owed', 'savings', 'cash', 'cashFree', 'total', 'totalReal', 'totalFree', 'salary', 'interest', 'pay', 'loanInterest', 'paid', 'paidOut'];
    const rows = Object.fromEntries(keys.map((k) => [k, []]));
    let deposits = balance;
    let savings = balance;
    let cash = 0; // with the loan
    let cashFree = 0; // without the loan
    let owed = loan;
    let paid = 0;
    let paidOut = 0;
    for (let m = 0; m <= months; m++) {
      let pay = 0;
      let charged = 0;
      let interest = 0;
      let wage = 0;
      if (m > 0) {
        wage = salary * (1 + raise) ** Math.floor((m - 1) / 12);
        interest = savings * (rate / 12);
        savings += interest + deposit;
        deposits += deposit;
        if (owed > 0 && m <= periods) {
          charged = owed * loanMonthly;
          pay = payment;
          owed = Math.max(0, owed - (pay - charged));
          if (owed < 0.005) owed = 0;
          paid += charged;
          paidOut += pay;
        }
        const left = wage - expenses - deposit;
        cash += left - pay;
        cashFree += left;
      }
      rows.xs.push(m);
      rows.deposits.push(deposits);
      rows.earned.push(savings - deposits);
      rows.owed.push(-owed);
      rows.savings.push(savings);
      rows.cash.push(cash);
      rows.cashFree.push(cashFree);
      rows.total.push(savings + cash);
      rows.totalReal.push((savings + cash) / (1 + inflation) ** (m / 12));
      rows.totalFree.push(savings + cashFree);
      rows.salary.push(wage);
      rows.interest.push(interest);
      rows.pay.push(pay);
      rows.loanInterest.push(charged);
      rows.paid.push(paid);
      rows.paidOut.push(paidOut);
    }
    const money = new NxsChart.Format('money', { unit });
    const series = [
      { name: 'Savings: balance and deposits', values: rows.deposits, slot: 1 },
      { name: 'Interest earned', values: rows.earned, slot: 2 },
    ];
    const both = loan > 0 ? 'Savings + cash, with the loan' : 'Savings + cash';
    const today = `${both}, in today's money`;
    const lines = [real ? { name: today, values: rows.totalReal } : { name: both, values: rows.total }];
    const extras = [
      real ? { name: both, values: rows.total } : { name: today, values: rows.totalReal },
      { name: 'Savings', values: rows.savings },
      { name: loan > 0 ? 'Cash, with the loan' : 'Cash', values: rows.cash },
      { name: 'Salary', values: rows.salary },
      { name: 'Interest this month', values: rows.interest },
    ];
    const summary = [
      [`Savings after ${months} months`, money.full(savings)],
      ['Interest earned', money.full(savings - deposits)],
      [loan > 0 ? 'Cash, with the loan' : 'Cash', money.full(cash)],
      [both, money.full(savings + cash)],
    ];
    if (real) summary.push([today, money.full(rows.totalReal[months])]);
    if (loan > 0) {
      series.push({ name: 'Loan balance', values: rows.owed, slot: 3 });
      extras.push(
        { name: 'Savings + cash, without the loan', values: rows.totalFree },
        { name: 'Loan payment', values: rows.pay },
        { name: 'Loan interest this month', values: rows.loanInterest },
        { name: 'Paid on the loan so far', values: rows.paidOut },
        { name: 'Loan interest paid so far', values: rows.paid },
      );
      summary.push(
        ['Savings + cash, without the loan', money.full(savings + cashFree)],
        ['Loan payment a month', money.full(payment)],
        ['Loan interest paid', money.full(paid)],
      );
    }
    const short = rows.cash.findIndex((v) => v < 0);
    if (short >= 0) {
      summary.push([`Cash below zero from month ${short}, lowest`, money.full(Math.min(...rows.cash))]);
    }
    return {
      chart: {
        xs: rows.xs,
        xTitle: 'Month',
        xLabels: rows.xs.map((m) => (m === 0 ? 'Start' : `Month ${m}, year ${Math.floor((m - 1) / 12) + 1}`)),
        x: { kind: 'plain' },
        y: { kind: 'money', unit },
        series,
        lines,
        extras,
        label: `Savings by month over ${months} months, the loan still owed, and savings plus cash with the loan payments`,
      },
      summary,
    };
  });
})();
