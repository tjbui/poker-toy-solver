import type { CardCode, Rank, Suit } from '../types/cards'

export const SUITS: Suit[] = ['S', 'H', 'C', 'D']
export const RANKS: Rank[] = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2']

export const ALL_CARDS: CardCode[] = SUITS.flatMap((suit) =>
  RANKS.map((rank) => `${suit}-${rank}` as CardCode),
)

export function rankToFileValue(rank: Rank): string {
  switch (rank) {
    case 'A':
      return '1'
    case 'T':
      return '10'
    case 'J':
      return '11'
    case 'Q':
      return '12'
    case 'K':
      return '13'
    default:
      return rank
  }
}

export function cardCodeToFilename(card: CardCode): string {
  const [suit, rank] = card.split('-') as [Suit, Rank]
  return `${suit}-${rankToFileValue(rank)}.png`
}