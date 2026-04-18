export type Suit = 'S' | 'H' | 'C' | 'D'
export type Rank =
  | 'A'
  | 'K'
  | 'Q'
  | 'J'
  | 'T'
  | '9'
  | '8'
  | '7'
  | '6'
  | '5'
  | '4'
  | '3'
  | '2'

export type CardCode = `${Suit}-${Rank}`
export type CardValue = CardCode | 'BACK' | null

export type CardSelectionTarget = {
  section: 'hero' | 'villain' | 'community'
  index: number
}