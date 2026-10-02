"""Abstract lifecycle contract only: no driver code, native return-code mapping,
addresses, packet bytes or fault reproduction. Not a tested native repair.

Return-code interpretation is deliberately absent: a future integration must
prove who owns each buffer before choosing a transition in this model.
"""
import unittest


class Ownership:
    def __init__(self):
        self.owner = {}
        self.returns = {}

    def acquire(self, token):
        if self.owner.get(token, 'pool') != 'pool':
            raise ValueError('already owned')
        self.owner[token] = 'dispatch'

    def observe(self, token):
        if self.owner.get(token) != 'dispatch':
            raise ValueError('observation outside dispatch lifetime')
        # An observer copies data; it does not acquire buffer ownership.

    def complete(self, token, disposition):
        if self.owner.get(token) != 'dispatch':
            raise ValueError('dispatch no longer owns token')
        if disposition == 'transferred_to_waiter':
            self.owner[token] = 'waiter'
        elif disposition in ('consumed_no_transfer', 'rejected_no_transfer'):
            self.owner[token] = 'pool'
            self.returns[token] = self.returns.get(token, 0) + 1
        else:
            # Ambiguity must be resolved, not converted to an automatic return.
            raise ValueError('ownership evidence required')

    def waiter_release(self, token):
        if self.owner.get(token) != 'waiter':
            raise ValueError('waiter does not own token')
        self.owner[token] = 'pool'
        self.returns[token] = self.returns.get(token, 0) + 1


class ContractTests(unittest.TestCase):
    def test_observation_never_returns_or_transfers(self):
        model = Ownership(); model.acquire('a')
        model.observe('a'); model.observe('a')
        self.assertEqual(model.owner['a'], 'dispatch')
        self.assertEqual(model.returns, {})

    def test_completed_packets_return_exactly_once(self):
        for disposition in ('consumed_no_transfer', 'rejected_no_transfer'):
            model = Ownership(); model.acquire('a')
            model.complete('a', disposition)
            with self.assertRaises(ValueError): model.complete('a', disposition)
            with self.assertRaises(ValueError): model.waiter_release('a')
            self.assertEqual(model.returns['a'], 1)

    def test_waiter_owns_until_release(self):
        model = Ownership(); model.acquire('a'); model.acquire('b')
        model.complete('a', 'transferred_to_waiter')
        model.complete('b', 'consumed_no_transfer')
        with self.assertRaises(ValueError): model.acquire('a')
        with self.assertRaises(ValueError): model.complete('a', 'rejected_no_transfer')
        with self.assertRaises(ValueError): model.observe('a')
        self.assertNotIn('a', model.returns)
        model.waiter_release('a')
        with self.assertRaises(ValueError): model.waiter_release('a')
        self.assertEqual(model.returns['a'], 1)

    def test_ambiguous_error_does_not_guess(self):
        model = Ownership(); model.acquire('a')
        with self.assertRaises(ValueError): model.complete('a', 'unknown_error')
        self.assertEqual(model.owner['a'], 'dispatch')
        self.assertEqual(model.returns, {})

    def test_repeated_mixed_lifetimes(self):
        model = Ownership()
        for sequence in range(1000):
            token = ('session', sequence)
            model.acquire(token); model.observe(token)
            if sequence % 3 == 0:
                model.complete(token, 'transferred_to_waiter'); model.waiter_release(token)
            else:
                model.complete(token, 'consumed_no_transfer')
            self.assertEqual(model.owner[token], 'pool')
            self.assertEqual(model.returns[token], 1)


if __name__ == '__main__':
    unittest.main()
