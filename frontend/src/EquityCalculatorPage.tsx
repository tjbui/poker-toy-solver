import { useMemo, useState } from 'react'
import CardPickerModal from './CardPickerModal'
import CardRow from './CardRow'
import TableSection from './TableSection'
import { calculateEquity } from './api'
import {
  buildEquityRequest,
  type CardCode,
  type CardSelectionTarget,
  type CardValue,
  type EquityResult,
} from './cards'

export default function EquityCalculatorPage() {
  const [heroCards, setHeroCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [villainCards, setVillainCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [communityCards, setCommunityCards] = useState<CardValue[]>([
    'BACK',
    'BACK',
    'BACK',
    'BACK',
    'BACK',
  ])

  const [activeTarget, setActiveTarget] = useState<CardSelectionTarget | null>(null)

  const [result, setResult] = useState<EquityResult | null>(null)
  const [isLoading, setIsLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  const [engine, setEngine] = useState<'cpu' | 'gpu'>('cpu')
  const [simulations, setSimulations] = useState(1000000)

  const usedCards = useMemo(() => {
    return [...heroCards, ...villainCards, ...communityCards].filter(
      (card): card is CardCode => card !== null && card !== 'BACK',
    )
  }, [heroCards, villainCards, communityCards])

  function handleCardClick(section: CardSelectionTarget['section'], index: number) {
    setActiveTarget({ section, index })
  }

  function handleSelectCard(value: CardValue) {
    if (!activeTarget) return

    const { section, index } = activeTarget

    if (section === 'hero') {
      const next = [...heroCards]
      next[index] = value
      setHeroCards(next)
    } else if (section === 'villain') {
      const next = [...villainCards]
      next[index] = value
      setVillainCards(next)
    } else {
      const next = [...communityCards]
      next[index] = value
      setCommunityCards(next)
    }

    setActiveTarget(null)
  }

  async function handleSimulate() {
    try {
      setIsLoading(true)
      setError(null)

      const request = buildEquityRequest(
        heroCards,
        villainCards,
        communityCards,
        simulations,
        engine,
      )

      const data = await calculateEquity(request)
      setResult(data)
    } catch (err) {
      console.error(err)
      setError('Failed to calculate equity.')
    } finally {
      setIsLoading(false)
    }
  }

  return (
    <div className="page">
      <div className="table-shell">
        <h1 className="title">Poker Equity Calculator</h1>

        <div className="table-layout">
          <div className="top-row">
            <TableSection title="Hero Cards">
              <CardRow
                cards={heroCards}
                onCardClick={(index) => handleCardClick('hero', index)}
              />
            </TableSection>

            <TableSection title="Community Cards">
              <CardRow
                cards={communityCards}
                onCardClick={(index) => handleCardClick('community', index)}
              />
            </TableSection>
          </div>

          <div className="bottom-layout">
            <TableSection title="Villain Cards">
              <CardRow
                cards={villainCards}
                onCardClick={(index) => handleCardClick('villain', index)}
              />
            </TableSection>

            <section className="side-panel">
              <div className="controls-row">
                <div className="control-group">
                  <label htmlFor="engine-select" className="control-label">
                    Engine
                  </label>
                  <select
                    id="engine-select"
                    className="control-input"
                    value={engine}
                    onChange={(e) => setEngine(e.target.value as 'cpu' | 'gpu')}
                  >
                    <option value="cpu">CPU</option>
                    <option value="gpu">GPU</option>
                  </select>
                </div>

                <div className="control-group">
                  <label htmlFor="simulations-input" className="control-label">
                    Simulations
                  </label>
                  <input
                    id="simulations-input"
                    className="control-input"
                    type="number"
                    min={1}
                    step={1000}
                    value={simulations}
                    onChange={(e) => setSimulations(Number(e.target.value))}
                  />
                </div>

                <button
                  className="simulate-button"
                  onClick={handleSimulate}
                  disabled={isLoading}
                  type="button"
                >
                  {isLoading ? 'Running Simulation...' : 'Calculate Equity'}
                </button>
              </div>

              <div className="results-panel">
                <h2 className="results-title">Results</h2>

                {error && <p className="error-text">{error}</p>}

                {!error && !result && !isLoading && (
                  <p className="placeholder-text">
                    Run a simulation to see equity results.
                  </p>
                )}

                {!error && result && (
                  <div className="results-grid">
                    <div className="result-card">
                      <span className="result-label">Hero Win %</span>
                      <span className="result-value">{result.heroWinPct.toFixed(2)}%</span>
                    </div>

                    <div className="result-card">
                      <span className="result-label">Villain Win %</span>
                      <span className="result-value">{result.villainWinPct.toFixed(2)}%</span>
                    </div>

                    <div className="result-card">
                      <span className="result-label">Tie %</span>
                      <span className="result-value">{result.tiePct.toFixed(2)}%</span>
                    </div>

                    <div className="result-card">
                      <span className="result-label">Hands Simulated</span>
                      <span className="result-value">
                        {result.simulatedHands.toLocaleString()}
                      </span>
                    </div>

                    <div className="result-card">
                      <span className="result-label">Runtime</span>
                      <span className="result-value">
                        {result.runtimeMs >= 1000
                          ? `${(result.runtimeMs / 1000).toFixed(2)} s`
                          : `${result.runtimeMs.toFixed(2)} ms`}
                      </span>
                    </div>

                    <div className="result-card">
                      <span className="result-label">Engine</span>
                      <span className="result-value engine-pill">{result.engine}</span>
                    </div>
                  </div>
                )}
              </div>

              <div className="github-row">
                <a
                  href="https://github.com/tjbui/poker-equity"
                  target="_blank"
                  rel="noreferrer"
                  className="github-link-icon"
                  aria-label="View GitHub Repository"
                  title="View GitHub Repository"
                >
                  <img
                    src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/github/github-original.svg"
                    alt="GitHub"
                    className="github-logo"
                  />
                </a>
              </div>
            </section>
          </div>
        </div>
      </div>

      <CardPickerModal
        isOpen={activeTarget !== null}
        usedCards={usedCards}
        onClose={() => setActiveTarget(null)}
        onSelect={handleSelectCard}
      />
    </div>
  )
}